/* pcm.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <sndfile.h>
#include "log.h"
#include "led.h"
#include "key.h"
#include "rec.h"
#include "press.h"
#include "conf.h"
#include "pcm.h"
#include "cmp.h"

static char strbuf[512];
extern int unshi_mode;
extern uint16_t mask_drone;
extern uint32_t press_std;

wav_pcm_t *mix_wav;
int pcm_id = 0;
unsigned int p_func_no = 0;

#include "pcm_data.h"

int read_file2( const char *in_fname, short** pptr, unsigned int* len)
/* Note:len is samples */
{
	SNDFILE		*sfp;
	SF_INFO		sfinfo;
	int		cnt;
	char path_wav[STR_MAX + 1];

	sfinfo.format = 0;
	sprintf(path_wav, "%s/%s.wav", WAV_DIR, in_fname);
	sprintf(strbuf, "path_wav:%s\n", path_wav);
 	//log_prt(strbuf);

	sfp = sf_open(path_wav, SFM_READ, &sfinfo);
	if (sfp == NULL){
		log_prt("sf_open(%s) err\n", path_wav);
		return -1;
	}
#if 0
	printf("cnt:%lld, rate:%d, ch:%d, fmt:0x%x\n",
	  sfinfo.frames, sfinfo.samplerate, sfinfo.channels,
	  sfinfo.format);
#endif

	*pptr = calloc( sfinfo.frames, sizeof(short) );
	if  (*pptr == NULL){
        	log_prt("pcm.c calloc err\n");
		sf_close(sfp);
        	return -2;
    	}
	cnt = sf_read_short(sfp, *pptr, sfinfo.frames);
	if  (cnt != sfinfo.frames){
        	log_prt("sfinfo.frames:%d vs cnt:%d\n", sfinfo.frames, cnt);
		sf_close(sfp);
        	return -3;
    	}
	//log_prt("read %d samples\n", cnt);
	*len = sfinfo.frames;
	sf_close(sfp);
	//log_prt("1st pcm:%02x\n", *(*pptr) );
	return 0;
}
int write_file( const char *out_fname, short* ptr, unsigned int len)
/* Note:len is samples */
{
	SNDFILE		*sfp;
	SF_INFO		sfinfo;
	int		cnt;
	char path_wav[STR_MAX + 1];

	sprintf(path_wav, "%s/%s.wav", WAV_S_DIR, out_fname);
	sprintf(strbuf, "path_wav:%s\n", path_wav);
 	//log_prt(strbuf);

	sfinfo.samplerate = 48000;
	sfinfo.channels = 1;
	sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_WAV;

	sfp = sf_open(path_wav, SFM_WRITE, &sfinfo);
	if (sfp == NULL){
		log_prt("sf_open(%s) err\n", out_fname);
		return -1;
	}
	cnt = sf_write_short(sfp, ptr, len); /* len is samples */
	if  (cnt != len){
        	log_prt("len:%d vs cnt:%d\n", len, cnt);
		sf_close(sfp);
        	return -3;
    	}
	sf_close(sfp);
	return 0;
}

#if 0
#define F_IN	"in.wav"
#define F_OUT	"out.wav"
int main(void)
{
	short *ptr;
	unsigned int len;

	printf("read:%d\n", read_file( F_IN, &ptr, &len) );
	printf("write:%d\n", write_file( F_OUT, ptr, len) );
	free(ptr);
	
	return 0;
}
#endif


short lag(int loc, double seisu, short *p_d, double shosu)
{
        const int x0=0, x1=1, x2=2, x3=3;
        double x, f;

        if ( seisu <= 1 || seisu >= loc - 2){
                /* hirei */
                return *p_d + ( *(p_d + 1) - *p_d ) * shosu;
        }
        /* Langurange */
        x = 2 + shosu;
        f = (x-x1)*(x-x2)*(x-x3) / ((x0-x1)*(x0-x2)*(x0-x3)) * (*(p_d-1));
        f += (x-x0)*(x-x2)*(x-x3) / ((x1-x0)*(x1-x2)*(x1-x3)) * (*p_d);
        f += (x-x0)*(x-x1)*(x-x3) / ((x2-x0)*(x2-x1)*(x2-x3)) * (*(p_d+1));
        f += (x-x0)*(x-x1)*(x-x2) / ((x3-x0)*(x3-x1)*(x3-x2)) * (*(p_d+2));
        return f;
}
int trim2zero_ad(wav_pcm_t* p)
{
	short *pp;
	int i;

	/* find zero cross from last sample */
	pp = p->ad_ptr + p->ad_cnt -1;
	//if ( *pp<=0 && *(pp-1)<=0 ) return 0;

	for (i=p->ad_cnt; i> 0; i--)
	{
		/* attach_decay start form positive */
		/* search "-" to "+" */
		if ( *(pp-1) < 0 && *pp >= 0 ){
			break;
		}
		pp--;
	} 
	log_prt("trim2zero:dec:%d\n", p->ad_cnt - (i-1) );
	p->ad_cnt = i - 1;
	return 0;
}
int trim2zero(wav_pcm_t* p)
{
	short *pp;
	int i;

	/* find zero cross from last sample */
	pp = p->ptr + p->cnt -1;
	//if ( *pp<=0 && *(pp-1)<=0 ) return 0;

	for (i=p->cnt; i> 0; i--)
	{
		/* sustain start form positive */
		/* search "-" to "+" */
		if ( *(pp-1) < 0 && *pp >= 0 ){
			break;
		}
		pp--;
	} 
	log_prt("trim2zero:dec:%d\n", p->cnt - (i-1) );
	p->cnt = i - 1;
	return 0;
}
int freq_calib(wav_pcm_t* p)
{
        double rate, pos, shosu, seisu, f_abs;
        int loc, i;
        short *wp, *wp1st, *p_d;

	f_abs = pcm_fn[p->w_idx].freq;
        if ( (f_abs <= 0.0)||(p->f_tgt <= 0.0) ) return 0;
        rate = p->f_tgt / f_abs;
        //attack
        loc = (double)p->ad_cnt / rate;
        wp = wp1st = (short*)calloc(loc, sizeof(short));
        if (wp == NULL) return 2;
        for ( i=0; i < loc; i++, wp++){
                pos = rate * i;
                shosu = modf(pos, &seisu);
                p_d = p->ad_ptr + (int)seisu;
		*wp = lag(loc, seisu, p_d, shosu);
        }
        free(p->ad_ptr);
        p->ad_ptr = wp1st;
        p->ad_cnt = loc;
	//trim2zero_ad(p);

        //sustain
        //expand sample cnt xxxx times
        wp = wp1st = (short*)calloc(p->cnt * EXPAND_SUS, sizeof(short));
        if (wp == NULL) return 4;
        for (i=0; i < EXPAND_SUS; i++){
                memcpy(wp, p->ptr, p->cnt * sizeof(short));
                wp += p->cnt;
        }
        free(p->ptr);
        p->ptr = wp1st;
        p->cnt *= EXPAND_SUS;

        //conversion
        loc = (double)p->cnt / rate - 1;
        wp = wp1st = (short*)calloc(loc, sizeof(short));
        if (wp == NULL) return 3;
        for ( i=0; i < loc; i++, wp++){
                pos = rate * i;
                shosu = modf(pos, &seisu);
                p_d = p->ptr + (int)seisu;
		*wp = lag(loc, seisu, p_d, shosu);
        }
        free(p->ptr);
        p->ptr = wp1st;
        p->cnt = loc;

	//trim2zero(p);

        return 0;
}

int pcm_read_cnt = 0;

/* pcm is little endian, cpu is little endian */
int pcm_attack_decay(wav_pcm_t *p)
{
	int ret;

	/* playback once */
	ret = *(p->ad_ptr + p->loc);
	(p->loc)++;
	if (p->loc >= p->ad_cnt){
		p->sts = S_SU;
		p->loc = 0;
		//log_prt("change to S_SU\n");
	} 
	return ret;
}
int pcm_release(wav_pcm_t *p)
{
        int ret;
        double fac_dyn;

#if 1
        /* fac is dynamic */
        fac_dyn = ((double)p->fac_rel * (p->re_dcnt - p->loc)) / p->re_dcnt;
#else
         /* convert from power ratio to amplitude (dynamic) */
         fac_dyn = (double)p->fac_rel *
 		sqrt(((double)p->re_dcnt - p->loc) / p->re_dcnt);
#endif
        /* playback uses sustain */
        ret = *(p->ptr + (p->loc % p->cnt) ) * fac_dyn;
        (p->loc)++;
        /* release is finished */
        if (p->loc >= p->re_dcnt){
                p->sts = S_AD;
                p->loc = 0;
                //log_prt("change to S_AD\n");
        }
        return ret;
}
int pcm_sustain(wav_pcm_t *p)
{
	int ret;

	/* playback: repeat  */
	ret = *(p->ptr + p->loc);
	(p->loc)++;
	if (p->loc >= p->cnt){
		p->loc = 0;
		//log_prt("continue S_SU\n");
	} 
	return ret;
}
void pcm_set_key(uint16_t key)
{
	int i;
	wav_pcm_t *p = &mix_wav[0];

	piLock(PCM_LOCK);
	for (i = 0; i < PCM_NUM; i++){
		p->prev_on = key & 0x1;
		key >>= 1;
		p++;
	}
	piUnlock(PCM_LOCK);
}
int pcm_read_each(uint16_t key, double fac)
{
	int i, cnt = 0, ret = 0, ret_rel = 0;
	int onoff;

	wav_pcm_t *p = &mix_wav[0];
	for (i = 0; i < PCM_NUM; i++){
		onoff = key & 0x1;
		key >>= 1;
		if(p->prev_on == S_ON && onoff == S_OFF){
			/* status release */
			p->sts = S_RE;
			/* continue location */
			/* p->loc = 0; */
			/* loud tone has long release time */
			p->fac_rel = fac;
			/* get release count(length) */
#if 1
			/* duration after key off */
			p->re_dcnt = p->re_cnt *
				( (fac < 256)? 1.0 : ((double)fac / 256));
			/* upper limit */
			if ( p->re_dcnt > SAMPLES_PER_MSEC * 400 ){
                                p->re_dcnt = SAMPLES_PER_MSEC * 400;
                        }
#else
			p->re_dcnt = p->re_cnt;
#endif

			//log_prt("change to S_RE\n");
		}
		if (onoff == S_ON){
			switch(p->sts){
			case S_AD:
			  ret += pcm_attack_decay(p);
			  cnt++;
			  break;
			case S_SU:
			  ret += pcm_sustain(p);
			  cnt++;
			  break;
			case S_RE:
			  p->sts = S_AD;
			  p->loc = 0;
			  ret += pcm_attack_decay(p);
			  cnt++;
			  break;
			}
		}else{ /* S_OFF */
			if (p->sts == S_RE){
				ret_rel += pcm_release(p);
				cnt++;
			}
		}
		p->prev_on = onoff;
		p++;
	}
	/* normalize */
	if (cnt != 0){
		ret  = (ret * fac) + ret_rel; /* add release */
		ret  >>= 10; /* 2^10 = 1024 */
	}
	ret = compress(fac, cnt, ret);
	if ( ret > INT16_MAX){
		ret = INT16_MAX;
		led_set(LED_OVR, LED_ON);
	} else if (ret < INT16_MIN){
		ret = INT16_MIN;
		led_set(LED_OVR, LED_ON);
	}
	if (ret !=0){
		//printf("ret:%d\n", ret);
	}
	rec_write(ret);
	return ret;
}

struct timeval prev_time;
long diff_max = 0;

void time_diff(void)
{
  struct timeval nowTime;
  long diff;

  gettimeofday(&nowTime, NULL);
  if (prev_time.tv_usec == 0){
    prev_time = nowTime;
    return;
  }
  if (prev_time.tv_usec > nowTime.tv_usec){
   diff = 1000000l - (prev_time.tv_usec - nowTime.tv_usec);
  }else{
   diff = nowTime.tv_usec - prev_time.tv_usec;
  }
  if (diff > diff_max){
	diff_max = diff;
	log_prt("re-new diff_max:%ld\n", diff_max);
  }
  prev_time = nowTime;
}

int pcm_read(int called, int count)
{
	static uint16_t key;
	double fac;

	if (count == 1){
		led_set(LED_OVR, LED_OFF);
	}
	/* get new data buf */
	if (called == 0){
		if (unshi_mode){
			key = read_key();
		}else{
			key = read_key() | mask_drone;
		}
		//test time_diff();
	}
	pcm_read_cnt++;
	press_amp_factor(&fac);

	return pcm_read_each(key, fac);
}

int pcm_free(void)
{
	int i;
	wav_pcm_t *p;

	//log_prt("pcm_free\n");
	for (i=1; i<=3; i++){
		led_set(i, LED_OFF);
	}
	for (i = 0; i < PCM_NUM; i++){
		p = &mix_wav[i];
		free(p->ptr);
		free(p->ad_ptr);
	}
	return 0;
}

double gain(int index, short total)
{
    return (double)index / (double)total;
}
int normalize(wav_pcm_t* p)
{
        int s_peak = 0;
        int e_peak = 0;
        short* pp;
        int i;
        double ratio;

        /* search peak from start */
        for (i=0, pp = p->ptr; i < PEAK_SMP; i++, pp++){
                if (s_peak < abs(*pp)){
                        s_peak = abs(*pp);
                }
        }
        /* search peak from end */
        for (i=0, pp = p->ptr + p->cnt -1 - PEAK_SMP;
          i < PEAK_SMP; i++, pp++){
                if (e_peak < abs(*pp)){
                        e_peak = abs(*pp);
                }
        }
        /* normalize */
        ratio = ((double)s_peak) / e_peak -1.0;
	log_prt("amp normalize ratio:%lf\n", ratio);
        for (i=0, pp = p->ptr; i < p->cnt; i++, pp++){
                *pp *= 1.0 + ratio * (double)i / p->cnt;
        }
        return 0;
}
int cross_fade(wav_pcm_t* p)
{
    if ( p->cnt & 0x1 ){ //make to odd
	p->cnt--;
    }
    //printf("cross:p->cnt:%d\n", p->cnt);

    // attack and decay
    short* sp;
    sp = calloc( p->ad_cnt + (p->cnt >> 1), 2 ); // ad + half of sustain
    memcpy(sp, p->ad_ptr, p->ad_cnt << 1); //copy from ad
    free(p->ad_ptr);
    memcpy(sp + p->ad_cnt, p->ptr, p->cnt); // half of sustain
    p->ad_ptr = sp;
    p->cnt >>= 1; // make half 
    p->ad_cnt += p->cnt; // + half of sustain
    // sustain
    short* pp;
    sp = p->ptr;
    int num = p->cnt;
    int i;
    pp = p->ptr + p->cnt;
    for (i = 0; i< num; i++){
      *sp = (short)( gain(i, num) * (*sp)
			+ gain(num - i, num) * (*pp) );
      sp++; pp++; 
    }

    return 0;
}
int amp(short* ptr, int cnt, int amp)
{
	int i;
	for (i = 0; i < cnt; i++){
		*ptr = ((double)(*ptr) * amp) / 100.0;
		ptr++;
	}
	return 0;
}
int pcm_init(void)
{
	int i;
	int ret = 0;
	unsigned int len;

	wav_pcm_t *p;

	//pcm_id = 0; /* Am */
	//pcm_id = 1; /* Fm */
	log_prt("key code(Am, Fm, etc):%d\n", pcm_id);
	mix_wav = pcm_data[pcm_id];

	for (i = 0; i < PCM_NUM; i++){
		//printf("pcm_init:i=%d\n", i);
		p = &mix_wav[i];
		ret = read_file2(pcm_fn[p->w_idx].s_fn, &p->ptr, &len );
		if (0 > ret ){
			log_prt("s_fn:error:%d, %d\n", i, ret );
			exit(1);
		}
		p->cnt = len;
                amp(p->ptr, p->cnt, p->amp);

		ret = read_file2(pcm_fn[p->w_idx].ad_fn, &p->ad_ptr, &len );
		if (0 > ret ){
			log_prt("ad_fn:error:%d, %d\n", i, ret);
			exit(-1);
		}
		p->ad_cnt = len;
                amp(p->ad_ptr, p->ad_cnt, p->amp);

 		/* Relase data are produced from sustain data */
                /* 440Hz sample in std time, low freq => long */
                p->re_cnt = SAMPLES_PER_MSEC*200*(440.0/p->f_tgt);

		if (p->cross){
			normalize(p);
			cross_fade(p);
		}
		freq_calib(p);
	}
	log_prt("pcm:init end\n");
	return 0;
}
void p_save_raw(void)
{
        int i;
        wav_pcm_t *p;

        pcm_init();
        p = &mix_wav[0];

        for (i = 0; i < PCM_NUM; i++, p++){
                //printf("pcm_save_raw:i=%d\n", i);
                write_file(pcm_fn[i].ad_fn, p->ad_ptr, p->ad_cnt );
                write_file(pcm_fn[i].s_fn, p->ptr, p->cnt );
        }
}

int pre_p_proc(void)
{
        int ret = 1;

        switch(p_func_no){
        case 1:
          p_save_raw();
          break;
        default:
          ret = 0;
          break;
        }
        return ret;
}


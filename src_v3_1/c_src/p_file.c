/* p_file.c */
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
#include "press.h"
#include "pcm.h"

extern int unshi_mode;
extern uint16_t mask_drone;
extern uint32_t press_std;

wav_pcm_t *mix_wav;
int pcm_id = 0;
unsigned int p_func_no = 0;

#include "pcm_data.h"

int p_pcm_get(void);

int read_file2( const char *in_fname, short** pptr, unsigned int* len)
/* Note:len is samples */
{
	SNDFILE		*sfp;
	SF_INFO		sfinfo;
	int		cnt;

	sfinfo.format = 0;

	sfp = sf_open(in_fname, SFM_READ, &sfinfo);
	if (sfp == NULL){
		log_prt("sf_open(%s) err\n", in_fname);
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

	sfinfo.samplerate = 48000;
	sfinfo.channels = 1;
	sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_WAV;

	sfp = sf_open(out_fname,  SFM_WRITE, &sfinfo);
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

        return 0;
}

int pcm_free(void)
{
	int i;
	wav_pcm_t *p;

	//log_prt("pcm_free\n");
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
    memcpy(sp, p->ad_ptr, p->ad_cnt<<1); //copy from ad
    free(p->ad_ptr);
    memcpy(sp + p->ad_cnt, p->ptr, p->cnt); //(short*, ..)half of sustain
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
	char path_wav[STR_MAX + 1];

	wav_pcm_t *p;

	log_prt("key code(Am, Fm, etc):%d\n", pcm_id);
	mix_wav = pcm_data[pcm_id];

	log_prt("pcm_init:pcm_id=%d\n", pcm_id);
	for (i = 0; i < PCM_NUM; i++){
		//printf("pcm_init:i=%d\n", i);
		p = &mix_wav[i];
	        sprintf(path_wav, "%s%d/%s.wav",
		  WAV_S_DIR, pcm_id, pcm_fn[p->w_idx].s_fn);
		ret = read_file2(path_wav, &p->ptr, &len );
		if (0 > ret ){
			log_prt("s_fn:error:%d, %d\n", i, ret );
			exit(1);
		}
		p->cnt = len;

	        sprintf(path_wav, "%s%d/%s.wav",
		  WAV_S_DIR, pcm_id, pcm_fn[p->w_idx].ad_fn);
		ret = read_file2(path_wav, &p->ad_ptr, &len );
		if (0 > ret ){
			log_prt("ad_fn:error:%d, %d\n", i, ret);
			exit(-1);
		}
		p->ad_cnt = len;
	}
	log_prt("pcm:init end\n");
	return 0;
}
int p_pcm_get(void)
{
	int i;
	int ret = 0;
	unsigned int len;
        char path_wav[STR_MAX + 1];

	wav_pcm_t *p;

	//pcm_id = 0; /* Am */
	//pcm_id = 1; /* Fm */
	log_prt("key code(Am, Fm, etc):%d\n", pcm_id);
	mix_wav = pcm_data[pcm_id];

	for (i = 0; i < PCM_NUM; i++){
		//printf("pcm_get:i=%d\n", i);
		p = &mix_wav[i];
        	sprintf(path_wav, "%s/%s.wav",
		  WAV_DIR, pcm_fn[p->w_idx].s_fn);
		ret = read_file2(path_wav, &p->ptr, &len );
		if (0 > ret ){
			log_prt("s_fn:error:%d, %d\n", i, ret );
			exit(1);
		}
		p->cnt = len;
                amp(p->ptr, p->cnt, p->amp);

        	sprintf(path_wav, "%s/%s.wav",
		  WAV_DIR, pcm_fn[p->w_idx].ad_fn);
		ret = read_file2(path_wav, &p->ad_ptr, &len );
		if (0 > ret ){
			log_prt("ad_fn:error:%d, %d\n", i, ret);
			exit(-1);
		}
		p->ad_cnt = len;
                amp(p->ad_ptr, p->ad_cnt, p->amp);

 		/* Relase data are produced from sustain data */
                /* 440Hz sample in std time, low freq => long */
                p->re_cnt = SAMPLES_PER_MSEC
				*( 100 + 200*(440.0/(p->f_tgt)) );

		if (p->cross){
			normalize(p);
			cross_fade(p);
		}
		freq_calib(p);
#if 1
		if (0==(p->cross)){
			cross_fade(p);
		}
#endif
	}
	return 0;
}
void p_save_raw(void)
{
        int i;
        wav_pcm_t *p;
	char path_wav[STR_MAX + 1];

        p = &mix_wav[0];

        for (i = 0; i < PCM_NUM; i++, p++){
                //printf("pcm_save_raw:i=%d\n", i);
		sprintf(path_wav, "%s%d/%s.wav",
			WAV_S_DIR, pcm_id, pcm_fn[p->w_idx].ad_fn);
                write_file(path_wav, p->ad_ptr, p->ad_cnt );

		sprintf(path_wav, "%s%d/%s.wav",
			WAV_S_DIR, pcm_id, pcm_fn[p->w_idx].s_fn);
                write_file(path_wav, p->ptr, p->cnt );
        }
}

int pre_p_proc(void)
{
        int ret = 1;

        switch(p_func_no){
        case 1:
	  for (pcm_id=0; pcm_id <= 6; pcm_id++){
            p_pcm_get();
            p_save_raw();
	    pcm_free();
	  }
          break;
        default:
          ret = 0;
          break;
        }
        return ret;
}


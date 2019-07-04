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
#include "log.h"
#include "led.h"
#include "key.h"
#include "rec.h"
#include "press.h"
#include "pcm.h"
#include "compress.h"

static char strbuf[512];
extern int unshi_mode;

wav_pcm_t mix_wav[] = {
        { "aR1.wav", "sR1.wav", "rR1.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0, 0, 1, 70},
        { "aR2.wav", "sR2.wav", "rR2.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { "aR3.wav", "sR3.wav", "rR3.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 80},
        { "aR4.wav", "sR4.wav", "rR4.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 70},
        { "aR5.wav", "sR5.wav", "rR5.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { "aR6.wav", "sR6.wav", "rR6.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 70},
        { "aR7.wav", "sR7.wav", "rR7.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 60},
        { "aR8.wav", "sR8.wav", "rR8.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 40},
        { "aL1.wav", "sL1.wav", "rL1.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 70},
        { "aL2.wav", "sL2.wav", "rL2.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 70},
        { "aL3.wav", "sL3.wav", "rL3.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { "aL4.wav", "sL4.wav", "rL4.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { "aL5.wav", "sL5.wav", "rL5.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { "aL6.wav", "sL6.wav", "rL6.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 80},
        { "aL7.wav", "sL7.wav", "rL7.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { "aL8.wav", "sL8.wav", "rL8.wav", NULL, NULL, NULL,
	S_AD,  0, 0, 0, 0, 0, S_OFF, 0, 0 ,0, 0, 50},
        { NULL, NULL, },
};

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
		//log_("change to S_SU\n");
	} 
	return ret;
}
int pcm_release(wav_pcm_t *p)
{
	int ret;

	/* playback once */
	ret = *(p->re_ptr + p->loc);
	(p->loc)++;
	/* release is finished */
	if (p->loc >= p->re_cnt){
		p->sts = S_AD;
		p->loc = 0;
		//log_("change to S_AD\n");
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
		//log_("continue S_SU\n");
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
int pcm_read_each(uint16_t key, int fac)
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
			p->loc = 0;
			p->fac_rel = fac;
			//log_("change to S_RE\n");
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
				ret_rel += pcm_release(p) * p->fac_rel;
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
#if 0
	/* normalize 2nd 
	4	0.6997747437
	5	0.4479792299
	6	0.3264564612
	7	0.2554019768
	8	0.2089897005
	9	0.1763938401
	10	0.1522973243
	*/
	if (cnt >= 4){
		ret *= 1.5 / pow((double)cnt-2.0, 1.1);
	}
#endif
	/* compresstion */
	//ret = compress(ret);

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
	printf("re-new diff_max:%ld\n", diff_max);
  }
  prev_time = nowTime;
}

int pcm_read(int called, int count)
{
	static uint16_t key;
	static int fac;

	if (count == 1){
		led_set(LED_OVR, LED_OFF);
	}
	/* get new data buf */
	if (called == 0){
		key = read_key();
		press_amp_factor(&fac);
		//test time_diff();
	}
	return pcm_read_each(key, fac);
}
int read_file( const char *in_fname, short** pptr, unsigned int* len)
{
    FILE *fp;
    struct stat stbuf;
    int fd;
    char path_wav[STR_MAX + 1];

    sprintf(path_wav, "%s/%s", WAV_DIR, in_fname);
    sprintf(strbuf, "path_wav:%s\n", path_wav);
    //log_(strbuf);

    fd = open(path_wav, O_RDONLY);
    if (fd == -1) {
        sprintf(strbuf, "can't open file : %s.\n", in_fname);
        log_str("can't open file : %s.\n", in_fname);
	return -1;
    }
    fp = fdopen(fd, "rb");
    if (fp == NULL){
        log_str("can't open file : %s.\n", in_fname);
	return -2;
    }
    if (fstat(fd, &stbuf) == -1){
        log_str("cant get file state : %s.\n", in_fname);
	return -3;
    }
    *len = stbuf.st_size - HEADER_SIZE;

    *pptr = calloc( *len, 1 );   
    if  (*pptr == NULL){
        fclose(fp);
        return -4;
    }
    fd = fseek(fp, HEADER_SIZE, SEEK_SET);
    if (fd == -1 ){
        log_str("cant fseek: %s.\n", in_fname);
        return -5;
    }
    fd = fread(*pptr, 1, *len, fp);
    if (fd < *len){
        log_str("cant get all file data: %s.\n", in_fname);
        return -6;
    }

    fclose(fp);
    //printf("calloc:%p\n", *pptr );
    return 0;
}

int pcm_free(void)
{
	int i;
	wav_pcm_t *p;

	//log_("pcm_free\n");
	for (i=1; i<=3; i++){
		led_set(i, LED_OFF);
	}
	for (i = 0; i < PCM_NUM; i++){
		p = &mix_wav[i];
		free(p->ptr);
		free(p->ad_ptr);
		free(p->re_ptr);
		p->fname = NULL;
	}
	return 0;
}

double gain(int index, short total)
{
    return (double)index / (double)total;
}
int cross_fade(wav_pcm_t* p)
{
    if ( p->cnt & 0x1 ){ //make to odd
	p->cnt--;
    }
    printf("cross:p->cnt:%d\n", p->cnt);

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

	for (i = 0; i < PCM_NUM; i++){
		p = &mix_wav[i];
		if (p->fname != NULL){
			ret = read_file(p->fname, &p->ptr, &len );
		}else{	/* no file name */
			ret = 0;
			len = 0; 
		}
		if (0 > ret ){
			sprintf(strbuf, "error:%d, %s\n", ret, p->fname);
			log_(strbuf);
			exit(1);
		}
		p->cnt = len >> 1; /* byte -> short */
                amp(p->ptr, p->cnt, p->amp);

		if (p->ad_fn != NULL){
			ret = read_file(p->ad_fn, &p->ad_ptr, &len );
		}else{	/* no file name */
			ret = 0;
			len = 0; 
		}
		if (0 > ret ){
			sprintf(strbuf, "error:%d, %s\n", ret, p->ad_fn);
			log_(strbuf);
			exit(-1);
		}
		p->ad_cnt = len >> 1; /* byte -> short */
                amp(p->ad_ptr, p->ad_cnt, p->amp);

		if (p->re_fn != NULL){
			ret = read_file(p->re_fn, &p->re_ptr, &len );
		}else{	/* no file name */
			ret = 0;
			len = 0; 
		}
		if (0 > ret ){
			sprintf(strbuf, "error:%d, %s\n", ret, p->re_fn);
			log_(strbuf);
			exit(-1);
		}
		p->re_cnt = len >> 1; /* byte -> short */
                amp(p->re_ptr, p->re_cnt, p->amp);

		if (p->cross){
			cross_fade(p);
		}
	}
	//log_("read file finished \n");
	return 0;
}

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
#include "pcm.h"
#include "cmp.h"

extern int unshi_mode;
extern uint16_t mask_drone;
extern uint32_t press_std;

extern wav_pcm_t *mix_wav;

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
        fac_dyn = ((double)p->fac_rel *
	  (p->re_dcnt - (p->loc - p->re_1stloc))) / p->re_dcnt;
#else
         /* convert from power ratio to amplitude (dynamic) */
         fac_dyn = (double)p->fac_rel *
 		sqrt(((double)p->re_dcnt - p->loc) / p->re_dcnt);
#endif
        /* playback uses sustain */
        ret = *(p->ptr + (p->loc % p->cnt) ) * fac_dyn;
        (p->loc)++;
        /* release is finished */
        if (p->loc >= p->re_dcnt + p->re_1stloc){
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
			/* keep 1st location */
			p->re_1stloc = p->loc;
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

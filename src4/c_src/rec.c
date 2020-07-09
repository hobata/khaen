/* rec.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sndfile.h>

#include "common.h"
#include "led.h"
#include "log.h"
#include "rec.h"
#include "conf.h"
#include "pcm.h"

static short *prec = NULL;
static short *ptmp16;
static uint32_t cnt; /* record num */
static int rec_flag = 0;

extern int rec_no;

int rec_num = 48000*60*REC_TIME_MIN; // short

static void rec_reset(void)
{
	ptmp16 = prec;
	cnt = 0;
}
void rec_init(void)
{
	if ( prec != NULL){
		log_prt("rec:duplicate calloc err\n");
		exit(-1);
	}
	prec = calloc(rec_num, 2); /* short */
	if (prec == NULL){
		log_prt("rec:calloc err\n");
		exit(-1);
	}
	rec_flag = 0;
	rec_reset();
}
int rec_sts(void)
{
	return rec_flag; /* rec status */
}

int write_file( const char *out_fname, short* ptr, unsigned int len)
/* Note:len is samples */
{
        SNDFILE         *sfp;
        SF_INFO         sfinfo;
        int             cnt;

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

void rec_start(void)
{
	rec_flag = 1; /* rec start */
	led_set(LED_REC, LED_ON);
	log_prt("rec_start\n");
}
void rec_write(int16_t val)
{
	if (1 != rec_flag){ /* not recording */
		return;
	}
	if (rec_num <= cnt ){
		log_prt("rec_end\n");
		led_set(LED_REC, LED_OFF);
		rec_flag = 2;
		return;
	}
	*ptmp16 = val;
	ptmp16++;
	cnt++;
}
char* get_rec_no(void)
{
	static int rec_1st = -1;
	static int rec_2nd = 0;
	static char fstr[32];

	if (rec_1st == -1){
		/* config file */
		rec_1st = rec_no;
		rec_1st++;
		conf_rec(rec_1st);
	}else{
	  rec_2nd++; 
	}
	sprintf(fstr, "%04d-%04d", rec_1st, rec_2nd);
	return fstr;
}
void rec_save(void)
{
	char fname[MAX_STR];

	if ( rec_num > cnt ){
		return;
	}
	led_set(LED_REC, LED_ON);
	log_prt("rec_save");
	sprintf(fname, "%s%s.wav", REC_DIR, get_rec_no());

	write_file(fname, prec, cnt );

	del_file(REC_DIR, REC_LIMIT_NUM);
	rec_reset();
	led_set(LED_REC, LED_OFF);
	rec_flag = 0;
}
void rec_free(void)
{
	free(prec);
}

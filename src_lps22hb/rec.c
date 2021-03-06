/* rec.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "led.h"
#include "log.h"
#include "rec.h"

static uint8_t *prec = NULL;
static int16_t *ptmp16;
static uint32_t cnt; /* record bytes */
static int rec_flag = 0;

static const uint8_t whdr[] = {
	'R', 'I', 'F', 'F',  0xa2, 0x0, 0x0, 0x0,
	'W', 'A', 'V', 'E',  'f', 'm', 't', ' ',
	0x10, 0x00, 0x00, 0x00,  0x01, 0x00, 0x01, 0x00, 
	0x80, 0xbb, 0x00, 0x00,  0x00, 0x77, 0x01, 0x00,
	0x02, 0x00, 0x10, 0x00,  'd', 'a', 't', 'a'
};
#define HDR_SIZE (sizeof(whdr)) /* not contain byte size */

#if 0
static int log_(char* str)
{
	printf(str);
}
#endif
static void rec_reset(void)
{
	ptmp16 = (int16_t*)(prec + HDR_SIZE) + (LEN_SIZE >> 1);
	cnt = 0;
}
void rec_init(void)
{
	if ( prec != NULL){
		log_("rec:dupleicate calloc err\n");
		exit(-1);
	}
	prec = calloc(REC_SIZE + HDR_SIZE + LEN_SIZE, 1);
	if (prec == NULL){
		log_("rec:calloc err\n");
		exit(-1);
	}
	memcpy(prec, whdr, sizeof(whdr));
	rec_flag = 0;
	rec_reset();
}
int rec_sts(void)
{
	return rec_flag; /* rec status */
}
void rec_start(void)
{
	rec_flag = 1; /* rec start */
	led_set(LED_REC, LED_ON);
	log_("rec_start\n");
}
void rec_write(int16_t val)
{
	if (1 != rec_flag){ /* not recording */
		return;
	}
	if (REC_SIZE <= cnt ){
		log_("rec_end\n");
		led_set(LED_REC, LED_OFF);
		rec_flag = 2;
		return;
	}
	*ptmp16 = val;
	ptmp16++;
	cnt += 2; /* 2bytes per sample */
}
char* get_rec_no(void)
{
	static int rec_1st = -1;
	static int rec_2nd = 0;
	static char fstr[32];
	FILE *fp;
	char fname[] = "./rec_no";
	char str[256];

	if (rec_1st == -1){
		fp = fopen(fname, "r");
		if (fp == NULL){
	 	  /* open error */
		  log_("rec:fopen err\n");
		  /* make 1st number */
		  rec_1st = 0;
		}else{
		  /* read val */
		  if (NULL != fgets(str, 256, fp) ){
		    rec_1st = (int)strtol(str, NULL, 10);
		  }else{
	  	  /* read err */
		    rec_1st = 0;
		  }
		  fclose(fp);
		}
		rec_1st++;
		/* overwrite */
		fp = fopen(fname, "w");
		if (fp != NULL){
	  	  sprintf(str, "%d", rec_1st);
	  	  fputs(str, fp);
	  	  fclose(fp);
		}
	}else{
	  rec_2nd++; 
	}
	sprintf(fstr, "%04d-%04d", rec_1st, rec_2nd);
	return fstr;
}
void rec_save(void)
{
	FILE *fp;
	char fname[MAX_STR];
	time_t	now = time(NULL);
	struct	tm *ts;

	if ( REC_SIZE > cnt ){
		return;
	}
	led_set(LED_REC, LED_ON);
	log_("rec_save");
	uint32_t *pu32 = (uint32_t*)(prec + HDR_SIZE);
	*pu32 = cnt; /* little endian */
#if 0
	ts = localtime(&now);
	sprintf(fname, "/rec/%04d%02d%02d_%02d%02d%02d.wav",
		ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
		ts->tm_hour, ts->tm_min, ts->tm_sec);
#else
	sprintf(fname, "/rec/%s.wav", get_rec_no());
#endif
	fp = fopen(fname, "w");
	if (fp == NULL){
		log_("rec:fopen err\n");
		exit(-1);
	}
	if ( !fwrite(prec, 1, cnt + HDR_SIZE + 4, fp)){ /* 4 is length(byte) */
		log_("rec:fwrite err\n");
		exit(-1);
	}
	fflush(fp);
	fclose(fp);
	rec_reset();
	led_set(LED_REC, LED_OFF);
	rec_flag = 0;
}
void rec_free(void)
{
	free(prec);
}
#if 0
/* test */
int main(void)
{
	int i;

	rec_init();
	rec_start();
	for ( i=0; i < REC_SIZE; i++){
		rec_write(i % 32768);
	}
	rec_save();
	rec_free();
}
#endif

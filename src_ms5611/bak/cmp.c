/* compress.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cmp.h"
#include "log.h"

int c_sts;

int att_cnt;
int rel_cnt;
int thr_num;
double cmp_rate;

void cmp_init(int att_msec, int rel_msec, int thr, int cmp_per)
{
	att_cnt = (att_msec * SAMP_RATE) / 1000;
	rel_cnt = (rel_msec * SAMP_RATE) / 1000;
	thr_num = thr;
	cmp_rate = (double)cmp_per / 100;
	c_sts = C_NORMAL;
	log_prt("end:cmp_init\n");
}

int get_amp(int new_val)
{
	static int c_buf[BUF_NUM];
	static int buf_index = 0;
	static int max_index = 0;
	static int flag_1st = 0;
	int i;

	if (!flag_1st){
		memset(c_buf, 0, sizeof(c_buf)); 
		flag_1st++;
	}

	if (max_index == buf_index){
		c_buf[buf_index] = new_val;
		/* get max scan */
		max_index = 0;
		for (i=0; i< BUF_NUM; i++){
			if (c_buf[max_index] < c_buf[i]){
				max_index = i;
			}	
		}
	}else if (c_buf[max_index] < new_val){
		max_index = buf_index;
	}
	c_buf[buf_index++] = new_val;
	buf_index %= BUF_NUM;
	return c_buf[max_index];
}

int compress(int val)
{
	static int c_sts = C_NORMAL;
	static int cnt;
	double add_ratio = 1.0;
	double now_amp;

	if (!val){
		return val;
	}

#if 1
	now_amp = get_amp(abs(val));
#else
	now_amp = abs(val); //test
#endif

	if ( c_sts == C_NORMAL && now_amp > thr_num ){
		//printf("ATTACK\n");
		c_sts = C_ATTACK;
		cnt = att_cnt;
	}else if ( c_sts == C_COMP && now_amp <= thr_num ){
		//printf("REL\n");
		c_sts = C_REL;
		cnt = rel_cnt;
	}
	if ( c_sts == C_COMP){
		add_ratio = cmp_rate;
	}
	if ( c_sts == C_ATTACK){
		add_ratio = 1.0 - (1.0 - cmp_rate)
		  * (double)(att_cnt - cnt) / att_cnt;
		cnt--;
		if (!cnt){
			//printf("COMP\n");
			c_sts = C_COMP;
		}
	}
	if ( c_sts == C_REL){
		add_ratio = cmp_rate + (1.0 - cmp_rate)
		  * (double)(rel_cnt - cnt) / rel_cnt;
		cnt--;
		if (!cnt){
			//printf("NORMAL\n");
			c_sts = C_NORMAL;
		}
	}
	//printf("%f, %f\n", new_amp, now_amp);
	val *= add_ratio;
	return val;
}

#if 0
int main(void)
{

	int i;

	cmp_init(1, 1, 64, 30);

	for (i=0 ; i < 256 ; i++){
		printf("%d, %d\n", i, compress(i));
	}
	for (; i > 0 ; i--){
		printf("%d, %d\n", i, compress(i));
	}

	return 0;
}
#endif

// key.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <sched.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "pcm.h"
#include "rec.h"
#include "log.h"
#include "key.h"
#include "press.h"
#include "log.h"

void t_init(void);
void get_t_val(uint16_t *pkey16);

extern int wiringpi_setup_flag;
extern char lbuf[];
volatile uint32_t press_val;
volatile uint16_t temp_val; 

static volatile uint16_t key_val = 0;

static void dis_chatter(int ch)
{
	uint8_t val = 0;
	uint16_t  onoff, i;
	static chatt_t ct[2][8];
	/* sts: 0:fix, 1:wait_antei */
	chatt_t *pct;
	static int first = 0;
	uint16_t key16, key8;
#if 0
	//test
	return;
#endif

	/* init */
	if (0 == first){
		memset(ct, 0, sizeof(ct));
		get_t_val(&key16);
		piLock(KEY_LOCK);
		key_val = key16;
		piUnlock(KEY_LOCK);
		first++;
	}else{
		piLock(KEY_LOCK);
		key16 = key_val;
		piUnlock(KEY_LOCK);
	}

	/* get raw data */
	switch(ch){
	case 0:
		get_t_val(&key16);
		key8 = key16 & 0xff;
		break;
	case 1:
		key8 = key16 >> 8;
		break;
	default:
		return;
	}
	//printf("ch:0x%d, key8:0x%x, val:0x%x\n", ch, key8, val);
	/* check onoff until KEY_CHATTERING_NUM */
	pct = &ct[ch][0];
	for ( i=0; i<8; i++){
		onoff = val & 0x1;
		if ( pct->prev != onoff){
			pct->prev = onoff;
			pct->num = 1;
			pct->sts = 1;
		}else if (pct->sts == 1){
			/* wait antei , prev == now */
			pct->num++;
			if (pct->num >= KEY_CHATTERING_NUM){
				/* new */
				pct->num = 0;
				pct->sts = 0;
				/* overwrite */
				key8 = (key8 & (~(0x1 << i))) | (onoff << i);
			}
		}
		val >>= 1;
		pct++;
	}
	if (ch == 0){
		key16 = (key16 & 0xff00) | key8;
	}else{
		key16 = (key16 & 0xff) | (key8 << 8);
	}
	piLock(KEY_LOCK);
	key_val = key16;
	piUnlock(KEY_LOCK);
}
PI_THREAD (key_loop)
{
	static int cnt = 0;
	static int loop = 0;
#if 0
	int sched = SCHED_RR;
	struct sched_param param;
        // Set realtime priority for this thread
        param.sched_priority
		= sched_get_priority_max(SCHED_RR) + 10;
        if (sched_setscheduler(0, sched, &param) < 0){
                perror("sched_setscheduler2");
   	}
#endif	
	while(1){
		cnt++;
		cnt %= 2000;
		if (cnt == 0){
			loop+=2;
			log_prt("cycle_2k:key %dk, press %dk, %d, %d\n",
				loop, loop, press_val, temp_val);
		}
		usleep(KEY_INT_USEC); //sleep inside get_val
		//get_val(); //press.c
		dis_chatter(0);	
		dis_chatter(1);	
		//usleep(KEY_INT_USEC); //sleep inside get_val
		//get_val(); //press.c
		//dis_chatter(1);	
		//dis_chatter(0);	
	}
}
int key_init(void)
{
	useconds_t usec = 1; /* 1us */

        if (wiringpi_setup_flag == 0){
                wiringPiSetup();
                wiringpi_setup_flag = 1;
        }
	/* touch sensor(I2C) init */
	t_init();
        /* create key thread */
        int x;
        x = piThreadCreate (key_loop) ;
        if (x != 0){
                log_prt("keyThread didn't startn");
		exit(1);
	}
	log_prt("end:key_init\n");
	return 0;
}
/* return dfference: 0 to 1000 msec */
int diff_ms(struct timeval *now, struct timeval *prev)
{
        /* calculate ms */
        long long nowl = (long long)now->tv_sec*1000 + (long long)now->tv_usec / 1000;
        long long prevl = (long long)prev->tv_sec*1000 + (long long)prev->tv_usec / 1000;

        if (nowl < prevl){
                if (now->tv_sec >= 2){
                        return 1000;
                }else{
                        return (long long)1000 + now->tv_usec/1000 - prev->tv_usec/1000;
                }
        }
        return nowl - prevl;
}
uint16_t read_key(void)
{
        uint16_t ret;

        piLock(KEY_LOCK);
        ret = key_val;
        piUnlock(KEY_LOCK);
        return ret;
}


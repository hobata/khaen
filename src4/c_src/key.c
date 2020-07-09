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

static void dis_chatter(void)
{
	uint16_t  onoff, i;
	static chatt_t ct[16];
	/* sts: 0:fix, 1:wait_antei */
	chatt_t *pct;
	static int first = 0;
	uint16_t val, key16;

	/* init */
	if (0 != first){
		piLock(KEY_LOCK);
		key16 = key_val;
		piUnlock(KEY_LOCK);
		/* get raw data */
		get_t_val(&val);
	}else{
		memset(ct, 0, sizeof(ct));
		first++;
		return;
	}
	//printf("key_val:0x%x, val:0x%x\n",key_val, val);
	/* check onoff until KEY_CHATTERING_NUM */
	pct = ct;
	for ( i=0; i<16; i++){
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
				key16 = (key16 & (~(0x1 << i))) | (onoff << i);
			}
		}
		val >>= 1;
		pct++;
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
		cnt %= 10000;
		if (cnt == 0){
			loop++;
			log_prt("cycle_10k:key %d, press %d, %d, %d\n",
				loop, loop, press_val, temp_val);
		}
		usleep(KEY_INT_USEC); //sleep inside get_val
		dis_chatter();	
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

uint16_t read_key(void)
{
        uint16_t ret;

        piLock(KEY_LOCK);
        ret = key_val;
        piUnlock(KEY_LOCK);
        return ret;
}


/* monitor.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <wiringPi.h>
#include "press.h"
#include "key.h"
#include "log.h"
#include "led.h"

extern int pcm_read_cnt;
extern uint32_t press_val;
extern uint32_t press_std;
extern uint32_t press_diff;
extern uint32_t press_diff_prev;

PI_THREAD (monitor)
{
	//int fac;
	//int cnt = 0;

        while(1){
        	delay(5234);
#if 0
		cnt++;
		if (cnt % 2){
        		led_set(3, LED_ON);
		}else{
        		led_set(3, LED_OFF);
		}
#else
		//printf("press_val:%d, diff:%d, diff_prev:%d\n",
		//	press_val, press_diff, press_diff_prev );
#endif
		//printf("raw:0x%x, key:0x%x\n", get_raw(), read_key());
		//printf("press diff:%d\n", press_read());
		//printf("temp:%d\n", temp_read());
		//press_amp_factor(&fac);
		//printf("press amp factor:%d\n", fac);
        }
}

void monitor_init(void)
{
        /* create timer thread */
        int x;
        x = piThreadCreate (monitor) ;
        if (x != 0){
                log_prt("timerThread didn't startn");
		exit(1);
	}
	//log_prt("end:monitor_init\n");
}


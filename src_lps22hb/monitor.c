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

PI_THREAD (monitor)
{
	//int fac;

        while(1){
        	delay(5000);
		//printf("raw:0x%x, key:0x%x\n", get_raw(), read_key());
		//printf("press diff:0x%x\n", press_read());
		printf("temp:%d\n", temp_read());
		//press_amp_factor(&fac);
		//printf("press amp factor:%d\n", fac);
        }
}

void monitor_init(void)
{
        /* create timer thread */
        int x;
        x = piThreadCreate (monitor) ;
        if (x != 0)
                printf ("timerThread didn't startn");
}


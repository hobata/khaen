#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "led.h"

int main(void)
{
	struct timespec req, rem;
	int i = 0, loop, j;

        req.tv_sec = 1;
        req.tv_nsec = 0;

	init_led();
	while(1){
		/* toggle display */
		for (j=1, loop = i; j<=3; j++){
			if (loop & 1) {
				set_led(j, LED_ON);
			} else {
				set_led(j, LED_OFF);
			}
			loop >>= 1; 
		}
		i++;
		nanosleep(&req, &rem);
	}
	return 0;
}

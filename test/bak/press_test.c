#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "press.h"

extern double press_val;
extern int16_t temp_val;

int main(void)
{
	struct timespec req, rem;

        req.tv_sec = 1;
        req.tv_nsec = 0;

	init_press();
	while(1){
		nanosleep(&req, &rem);
		printf("press:%f, temp:%d\n", press_val, temp_val);
	}
	return 0;
}

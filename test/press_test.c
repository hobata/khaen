#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "press.h"

int main(void)
{
	struct timespec req, rem;

        req.tv_sec = 1;
        req.tv_nsec = 0;

	press_init();
	while(1){
		nanosleep(&req, &rem);
		printf("press diff:0x%x\n", press_read());
	}
	return 0;
}

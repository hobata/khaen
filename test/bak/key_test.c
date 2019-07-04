#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "key.h"

int main(void)
{
	struct timespec req, rem;

        req.tv_sec = 1;
        req.tv_nsec = 0;

	key_init();
	while(1){
		nanosleep(&req, &rem);
		printf("key_val:0x%x\n", read_key());
	}
	return 0;
}

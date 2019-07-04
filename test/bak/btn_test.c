#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern int8_t btnRed, btnGreen, btnBlue, btnYellow;

int main(void)
{
	struct timespec req, rem;
	int i = 0;

        req.tv_sec = 1;
        req.tv_nsec = 0;

	init_btn();
	while(1){
		nanosleep(&req, &rem);
		printf("%04d:btn:0x%x, 0x%x, 0x%x, 0x%x\n",
			i++, btnRed, btnGreen, btnBlue, btnYellow);
	}
	return 0;
}

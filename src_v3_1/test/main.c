#include <stdio.h>
#include <wiringPiI2C.h>

int main(void)
{
	int fd[2];
	int i, ret;

	for (i=0; i<2; i++)
	{
		fd[i] = wiringPiI2CSetup (i);
		ret = wiringPiI2CReadReg8 (fd[i], 0x7a); //device_id
		printf("fd[%d]:%d, DEV_ID:0x%x\n", i, fd[i], ret);
	}

	return 0;
}

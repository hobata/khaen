// control pressure sensor
// press.c
// 2017.12.28

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <wiringPi.h>
#include "press.h"

static char strbuf[512];

volatile double press_val = 0.0;
volatile int16_t temp_val = 0;

int readSPI(unsigned char addr, unsigned char *data);
int writeSPI(unsigned char addr, unsigned char data);

int readSPI(unsigned char addr, unsigned char *data)
{
	unsigned char array[2];
	int ret;

	array[0] = addr | 0x80; //set 0x80 bit 
	array[1] = 0x0;
	ret = wiringPiSPIDataRW (PRESS_CH, array, 2);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}
	*data = array[1]; 

	return 0;
}

int writeSPI(unsigned char addr, unsigned char data)
{
	unsigned char array[2];
	int ret;

	array[0] = addr & 0x7f; //clear 0x80 bit 
	array[1] = data;
	ret = wiringPiSPIDataRW (PRESS_CH, array, 2);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}

	return 0;
}

int readSPIm(unsigned char addr, unsigned char *data, int len)
{
	int i, ret;
	
	addr |= 0x80;	// set Read bit
	for (i=0; i < len; i++){
		data[i] = addr++;
	}
	data[i] = 0x0;
	ret = wiringPiSPIDataRW (PRESS_CH, data, len+1);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}

	return 0;
} 

int check_id(void)
{
	unsigned char val;

	/* check ID */ 
	readSPI(WHO_AM_I, &val);
	if (val != 0xb1){
		sprintf(strbuf, "WHO_AM_I mismatch:%02x\n", val);
		//log_(strbuf);
		printf("%s\n", strbuf);
	}else{
		sprintf(strbuf, "WHO_AM_I matched:%02x\n", val);
		//log_(strbuf);
		printf("%s\n", strbuf);
	}
	return 0;
}

int init_press(void)
{
	int ret, fd;
	unsigned char val;

	wiringPiSetup();
	fd =  wiringPiSPISetup (PRESS_CH, 10000000) ; //10MHz
	if (fd == -1){
		sprintf(strbuf, "wiringPiSPISetup:errno:%d\n", errno);
		//log_(strbuf);
		return 1;
	}else{
		sprintf(strbuf, "wiringPiSPISetup:fd:%d\n", fd);
		//log_(strbuf);
	}
	check_id();

	/* BOOT */
	val =  0x1 << 7;
	writeSPI(CTRL_REG2,val);
	do {
		readSPI(CTRL_REG2, &val);
	}while((val & 0x80) != 0);
	/* software reset */
	val =  0x1 << 2;
	writeSPI(CTRL_REG2,val);
	do {
		readSPI(CTRL_REG2, &val);
	}while((val & 0x4) != 0);
	/* INTERRUPT_CFG */
	//val =  0x4; /* LIR = 1, other = 0 */
	writeSPI(INT_CFG, val);
	/* CTRL_REG1 */
	val =  0x5 << 4; /* ODR = 101 ,EN_LPFP=0, LPFP_CFG=0 */
	writeSPI(CTRL_REG1, val);
	/* CTRL_REG2 */
	val = 0x1 << 3; /* I2C_DIS = 1 */
	val |= 0x1 << 4; /* IF_ADD_INC = 1 */
	writeSPI(CTRL_REG2, val);
	/* CTRL_REG3 DRDY = 1 */
	val =  0x1 << 2;
	writeSPI(CTRL_REG3, val);
	/* FIFO_CTRL = default */
	/* set intr handling */
	fd = wiringPiISR(PRESS_INIT_PIN, INT_EDGE_RISING, &intr_handler);
	if (fd < 0){
		sprintf(strbuf, "wiringPiISR:errno:%d\n", errno);
		//log_(strbuf);
		return 1;
	}
	/* dummy read */
	readSPI(STATUS, &val);
	while(val & 0x01){ /* Pressure data avaiable */
		mea_val();
		readSPI(STATUS, &val);
	} 

	printf("init done.\n");
	return 0;
}

int read_press_temp(uint32_t *ppress, uint16_t *ptemp)
{
	unsigned char sts;
        unsigned char array[6];
	int cnt = 0; 
        struct timespec req, rem;

        readSPIm(PRESS_OUT_XL, array, 5); /* press and temp */
        *ppress = array[1] | (uint32_t)array[2] << 8
                | (uint32_t)array[3] << 16 ;
        *ptemp =  array[4] | ( (uint16_t)array[5] << 8 );

	return 0;
}

int mea_val(void)
{
	uint32_t	press; //std value  1017*100*4096
	int16_t		temp;
	int amp_factor;

	read_press_temp(&press, &temp);
        press_val = (double)press/4096;
        temp_val = temp;

	return 0;
}

void intr_handler(void)
{
	unsigned char val;
	static int i = 0;

	//printf("call interrupt:%d\n", i++);
	//check_id(); //working
	readSPI(INT_SOURCE, &val);
	mea_val();
}

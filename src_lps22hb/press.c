// control pressure sensor
// press.c
// 2017.12.28

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "led.h"
#include "btn.h"
#include "press.h"
#include "monitor.h"
#include "log.h"
#include "key.h"
#include "rec.h"
#include "pcm.h"
#include "cos_tbl.h"
#include "compress.h"

static char strbuf[512];
extern int wiringpi_setup_flag;

volatile uint32_t press_val = 0; /* get measured pressure */
volatile uint32_t press_diff = 0;
volatile uint32_t press_std = 0;
volatile int	press_std_flag = 0; /* get standard pressure */
volatile uint16_t temp_val = 0; /* get measured temperature */

extern int unshi_mode;

static int readSPI(unsigned char addr, unsigned char *data);
static int writeSPI(unsigned char addr, unsigned char data);
static int get_val(void);

int press_amp_factor(int* pfac)
{
	int tmp;

	if (unshi_mode){
		*pfac = PRESS_UNSHI_FAC;
		return 0;
	}
	piLock(PRESS_LOCK);
	/* 1 hPa = 4096 count, factor max is 1024 */
	/* *pfac = (press_diff / 4096) / (2^PRESS_MAX) * 1024; */
	tmp = press_diff >> (2 + PRESS_MAX);
	piUnlock(PRESS_LOCK);

	/* human ear adjustment */
#if 1
	/* offset curve */
	if (tmp < PRESS_OFFSET){
		tmp = 0;
	} else if (tmp <= 1024-PRESS_OFFSET ){
		tmp = ((tmp-PRESS_OFFSET)<<10)/(1024 - PRESS_OFFSET*2);
	} else tmp = 1024;
#else
	/* cosine table */
	if ( tmp < 0){
		tmp = 0;
	} else if ( tmp >= 1024){
		tmp = 1024;
	} else if (tmp >= 0 && tmp <= 1023){
		tmp = cos_tbl[tmp];
	} 
#endif

	*pfac = tmp;
	return 0;
}
static int readSPI(unsigned char addr, unsigned char *data)
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

static int writeSPI(unsigned char addr, unsigned char data)
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

static int readSPIm(unsigned char addr, unsigned char *data, int len)
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

static int check_id(void)
{
	unsigned char val = 0;

	/* check ID */ 
	readSPI(WHO_AM_I, &val);
	if (val != 0xb1){
		sprintf(strbuf, "WHO_AM_I mismatch:%02x\n", val);
		log_(strbuf);
		//printf("%s\n", strbuf);
	}else{
		sprintf(strbuf, "WHO_AM_I matched:%02x\n", val);
		//log_(strbuf);
		//printf("%s\n", strbuf);
	}
	return 0;
}
void press_set_auto_zero(int flag)
{
	if (read_key()){
		return;
	}
	piLock(PRESS_LOCK);
	press_std_flag = 1 | flag;
	piUnlock(PRESS_LOCK);
	led_set(LED_ZERO, LED_ON);
}
PI_THREAD (autoZero)
{
	while(1){
		delay(PRESS_AUTO_ZERO_SEC * 1000);
		press_set_auto_zero(0); /* no force */
	}
}

int press_init(void)
{
	int fd;
	unsigned char val;

	led_init();
	btn_init();
	key_init();
	pcm_init();
	rec_init();
	monitor_init();
	cmp_init(10, 10, 32768/8*5, 30);

        if (wiringpi_setup_flag == 0){
                wiringPiSetup();
                wiringpi_setup_flag = 1;
        }
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
	//val =  (0x5 << 4) | ( 0x2 << 2); /* ODR = 101 ,EN_LPFP=1, LPFP_CFG=0 */
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
	fd = wiringPiISR(PRESS_INIT_PIN, INT_EDGE_RISING, &press_intr_handler);
	if (fd < 0){
		sprintf(strbuf, "wiringPiISR:errno:%d\n", errno);
		//log_(strbuf);
		return 1;
	}
	/* dummy read */
	readSPI(STATUS, &val);
	while(val & 0x01){ /* Pressure data avaiable */
		get_val();
		readSPI(STATUS, &val);
	}
#ifndef TEST_MODE 
		/* set flag to get standard value(force)*/
		delay(200); /* 1/(75/9) > 0.2 not good */
#endif
	press_set_auto_zero(PRESS_AUTO_DIFF_FORCE);
	/* autozero create timer thread */
	int x;
	x = piThreadCreate (autoZero) ;
	if (x != 0){
  		log_("timerThread didn't startn");
		exit(1);
	}

	//printf("init done.\n");
	return 0;
}

static int read_press_temp(uint32_t *ppress, uint16_t *ptemp)
{
        unsigned char array[6];

        readSPIm(PRESS_OUT_XL, array, 5); /* press and temp */
        *ppress = array[1] | (uint32_t)array[2] << 8
                | (uint32_t)array[3] << 16 ;
        *ptemp =  array[4] | ( (uint16_t)array[5] << 8 );

	return 0;
}

/* read diff value */
uint32_t press_read(void)
{
	uint32_t tmp;

	piLock(PRESS_LOCK);
        tmp = press_diff;
	piUnlock(PRESS_LOCK);
	return tmp;
}
uint16_t temp_read(void)
{
	uint16_t tmp;

	piLock(PRESS_LOCK);
        tmp = temp_val;
	piUnlock(PRESS_LOCK);
	return tmp;
}
static uint32_t press_hist(int mea, int std)
{
	if ( mea > std){
	  return (uint32_t)(mea - std);
	}else{
	  return (uint32_t)( (double)(std - mea) * 1.2 );
	}
}

static int get_val(void) /* get value from device */
{
	uint32_t	press; //pressure value(hPa)  1017*100*4096
	uint16_t	temp;  // temperature

	read_press_temp(&press, &temp);
	temp_val = temp; 
	//printf("press:0x%x\n", press); /* debug print */
	/* set standard value and diff */
	piLock(PRESS_LOCK);
	/* renew standard value */
	if ( press_std_flag ){
		//printf("set standard, press:0x%x\n", press);
		if ( (PRESS_AUTO_DIFF_MAX < abs((int)press - press_std))
			|| (press_std_flag & PRESS_AUTO_DIFF_FORCE) ){
			/* reset diff */
			//printf("reset diff:0x%x\n", press);
			press_std = press;
        		press_diff = 0;
		}else{
			/* discard */
			press_diff = press_hist(press, press_std);
		}
		press_std_flag = 0;
		led_set(LED_ZERO, LED_OFF);
	}else{
		press_diff = press_hist(press, press_std);
	}
        press_val = press;
	piUnlock(PRESS_LOCK);

	return 0;
}

void press_intr_handler(void)
{
#if 0
	unsigned char val;
	static int i = 0;

	printf("call interrupt:%d\n", i++);
	check_id(); //working
	readSPI(INT_SOURCE, &val);
#endif
	get_val();
}

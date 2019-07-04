// control pressure sensor
// press.c
// 2017.12.28

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <wiringPi.h>
#include "key.h"

static char strbuf[512];
static volatile uint16_t key_val = 0;

static int readSPIkey(unsigned char addr, unsigned char reg, unsigned char *data);
static int writeSPIkey(unsigned char addr, unsigned char reg, unsigned char data);
int key_init(void);
static int get_key(void);
uint16_t read_key(void);

static int readSPIkey(unsigned char addr, unsigned char reg, unsigned char *data)
{
	unsigned char array[4];
	int ret;

	array[0] = (addr << 1) | 0x40 | 0x1; // SPI control byte
	array[1] = reg;
	array[2] = 0;
	ret = wiringPiSPIDataRW (KEY_CH, array, 3);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}
	*data = array[2]; 

	return 0;
}

static int writeSPIkey(unsigned char addr, unsigned char reg, unsigned char data)
{
	unsigned char array[4];
	int ret;

	array[0] = (addr << 1) | 0x40; // SPI control byte
	array[1] = reg;
	array[2] = data;
	array[3] = 0;
	ret = wiringPiSPIDataRW (KEY_CH, array, 3);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}

	return 0;
}

static unsigned char swapbit(unsigned char x, unsigned char b) {
  unsigned r = 0;
  while (b--) {
    r <<= 1;
    r |= (x & 1);
    x >>= 1;
  }
  return r;
}

uint16_t read_key(void)
{
	uint16_t ret;

	piLock(KEY_LOCK);
	ret = key_val;
	piUnlock(KEY_LOCK);
	return ret;
}

static int get_key(void)
{
	/* I/O Expender is MCP23S17 */
	/* key:R1,Exp_GPA0 ... R8,Exp_GPA7 */
	/* key:L1,Exp_GPB7 ... L8,Exp_GPB0 */
	/* target:key_val(MSB-LSB) = B7...B0 A7...A0 */
	unsigned char data[2];
	uint16_t tmp16;

	readSPIkey(KEY_ADDR, GPIOA, &data[0]);
	readSPIkey(KEY_ADDR, GPIOB, &data[1]);

	/* swap bit */
	data[1] = swapbit(data[1], 8);
	tmp16 = (((uint16_t)data[1]) << 8) | data[0];
	piLock(KEY_LOCK);
	key_val = tmp16;
	piUnlock(KEY_LOCK);

	return 0;
}

int key_init(void)
{
	int ret, fd, i;
	unsigned char val;

	wiringPiSetup();
	fd =  wiringPiSPISetup (KEY_CH, 10000000) ; //10MHz
	if (fd == -1){
		sprintf(strbuf, "wiringPiSPISetup:errno:%d\n", errno);
		//log_(strbuf);
		return 1;
	}else{
		sprintf(strbuf, "wiringPiSPISetup:fd:%d\n", fd);
		//log_(strbuf);
	}
	/* IOCON MIRROR=1, INTPOL=1 */
	val = 0x40 | 0x2;
	writeSPIkey(KEY_ADDR, IOCON, val);
	/* GPINTEN all:1:intr enalble */
	val = 0xff;
	writeSPIkey(KEY_ADDR, GPINTENA, val);
	writeSPIkey(KEY_ADDR, GPINTENB, val);

	/* check */
	readSPIkey(KEY_ADDR, IODIRA, &val);
	printf("chek IODIRA:0x%x\n", val);

	/* set intr handling */
	fd = wiringPiISR(KEY_INT_PIN, INT_EDGE_RISING, &intr_handler_key);
	if (fd < 0){
		sprintf(strbuf, "wiringPiISR:errno:%d\n", errno);
		//log_(strbuf);
		return 1;
	}

	printf("init done.\n");
	return 0;
}

void intr_handler_key(void)
{
	unsigned char val;
	static int i = 0;

	//printf("call interrupt:%d\n", i++);
	get_key();
}

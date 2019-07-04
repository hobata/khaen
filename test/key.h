/* key.h */
#include <stdint.h>

#ifndef _KEY_H_
#define _KEY_H_

#define KEY_CH  1
#define KEY_INT_PIN 7
#define KEY_RESET_PIN 27
#define KEY_ADDR 0x0	/* A0, A1, A2 */
#define KEY_LOCK	0

#define AB_OFT		0x1	/* BANK = 0 */
/* register */
#define IODIRA		0x00
#define IODIRB		(IODIRA + AB_OFT)
#define IPOLA	        0x02
#define IPOLB	        (IPOLA + AB_OFT)
#define GPINTENA        0x04
#define GPINTENB	(GPINTENA + AB_OFT)
#define IOCON	        0x0a
#define GPPUA	        0x0c
#define GPPUB      	(GPPUA + AB_OFT) 
#define INTFA	        0x0e
#define INTFB      	(INTFA + AB_OFT) 
#define GPIOA	        0x12
#define GPIOB      	(GPIOA + AB_OFT) 
#define OLATA	        0x14
#define OLATB      	(OLATA + AB_OFT) 

static int readSPIkey(unsigned char addr, unsigned char reg, unsigned char *data);
static int writeSPIkey(unsigned char addr, unsigned char reg, unsigned char data);
int key_init(void);
static int get_key(void);
uint16_t read_key(void);
void intr_handler_key(void);

#endif /* _KEY_H */

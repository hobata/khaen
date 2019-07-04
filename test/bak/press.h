/* press.h */
#include <stdint.h>

#ifndef _PRESS_H_
#define _PRESS_H_

#define PRESS_CH  0
#define PRESS_INIT_PIN 6

/* register */
#define INT_CFG		0x0b
#define WHO_AM_I        0x0f
#define CTRL_REG1       0x10
#define CTRL_REG2       0x11
#define CTRL_REG3       0x12
#define FIFO_CTRL       0x14
#define INT_SOURCE      0x25
#define FIFO_STATUS     0x26
#define STATUS          0x27
#define PRESS_OUT_XL    0x28
#define PRESS_OUT_L     0x29
#define PRESS_OUT_H     0x2a
#define TEMP_OUT_L      0x2b
#define TEMP_OUT_H      0x2c

int init_press(void);
int readSPI(unsigned char addr, unsigned char *data);
int writeSPI(unsigned char addr, unsigned char data);
int press_init(void);
int set_stream(void);
int read_press_temp(uint32_t *ppress, uint16_t *ptemp);
int mea_val(void);
void intr_handler(void);

#endif /* _PRESS_H */

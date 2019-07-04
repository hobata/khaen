/* press.h */
#include <stdint.h>

#ifndef _PRESS_H_
#define _PRESS_H_

//#define TEST_MODE

#define PRESS_CH  0
#define PRESS_INIT_PIN 6
#define PRESS_LOCK 1

#define PRESS_AUTO_ZERO_SEC 60
#define PRESS_AUTO_DIFF_MAX 1024
#define PRESS_AUTO_DIFF_FORCE 2
#define PRESS_MAX (4)  // 16(2^4) hPa
#define PRESS_UNSHI_FAC	192 /* x / max(1024) */
#define PRESS_OFFSET	64 /* zero or max width */

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

int press_init(void);
void press_intr_handler(void);
uint32_t press_read(void);
uint16_t temp_read(void);
void press_set_auto_zero(int);
int press_amp_factor(int* pfac);

#endif /* _PRESS_H */

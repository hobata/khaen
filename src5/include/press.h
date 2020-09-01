/* press.h */
#include <stdint.h>

#ifndef _PRESS_H_
#define _PRESS_H_

//#define TEST_MODE

#define PRESS_CH  0
#define PRESS_LOCK 1

#define PRESS_AUTO_ZERO_SEC 60
#define PRESS_AUTO_DIFF_MAX 50 /* 50 pascal /min */
#define PRESS_AUTO_DIFF_FORCE 2
#define PRESS_MAX 1800  // xx Pa
#define PRESS_UNSHI_FAC	64 /* x / max(1024) */
#define PRESS_OFFSET	50 /* zero or max width */
#define PCM_FULL_CNT	16 /* PCM data num between presssure mea. interval */
#define WAIT_PRESS_USEC	2000

int press_init(void);
uint32_t press_read(void);
uint16_t temp_read(void);
void press_set_auto_zero(int);
int press_amp_factor(double* pfac);
int get_val(void);

/* register */
#define MS_RESET	0x1e
#define MS_C_D1_256	0x40
#define MS_C_D1_512	0x42
#define MS_C_D2_256	0x50
#define MS_ADC		0x00
#define MS_PROM_0	0xa0
#define MS_PROM_2	0xa2
#define MS_PROM_4	0xa4
#define MS_PROM_6	0xa6
#define MS_PROM_8	0xa8
#define MS_PROM_A	0xaa
#define MS_PROM_C	0xac
#define MS_PROM_E	0xae

typedef struct {
    uint16_t c1, c2, c3, c4, c5, c6;
} CAL_DATA_T;

#endif /* _PRESS_H */

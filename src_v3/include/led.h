/* LED.h */

#ifndef _LED_H_
#define _LED_H_

#define LED_NUM 3

//wPi No.
#define PIN_NO1	0
#define PIN_NO2 2	
#define PIN_NO3 3
#define PIN_PWR 25

#define LED_ON	1
#define LED_OFF	0

#define LED_ZERO	1
#define LED_OVR		1
#define LED_REC		2
#define LED_UNSHI	3

int led_init(void);
int led_set(unsigned char no, unsigned char on_off);

#endif /* _LED_H_ */

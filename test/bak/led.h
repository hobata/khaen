/* LED.h */

#ifndef _LED_H_
#define _LED_H_

//wPi No.
#define PIN_NO1	3
#define PIN_NO2 2	
#define PIN_NO3 0	

#define LED_ON	1
#define LED_OFF	0

int init_led(void);
int set_led(unsigned char no, unsigned char on_off);

#endif /* _LED_H_ */

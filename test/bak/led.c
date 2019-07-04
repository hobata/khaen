#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include "led.h"

int set_led(unsigned char no, unsigned char value)
{
	switch(no){
	case 1:
		no = PIN_NO1;
		break;
	case 2:
		no = PIN_NO2;
		break;
	case 3:
		no = PIN_NO3;
		break;
	default:
		printf("wrong pin no=%d\n", no);
	}
	if (value == LED_OFF){
		/* OFF */
		digitalWrite(no, HIGH);
	}else{	/* ON */
		digitalWrite(no, LOW);
	}
}

int init_led(void)
{
	wiringPiSetup();

	/* set initial value */
	pinMode (PIN_NO1, OUTPUT);
	pinMode (PIN_NO2, OUTPUT);
	pinMode (PIN_NO3, OUTPUT);
	/* light off */
	set_led(1, LED_OFF);
	set_led(2, LED_OFF);
	set_led(3, LED_OFF);

	return 0;
}

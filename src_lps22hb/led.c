#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include "led.h"

extern int wiringpi_setup_flag;

int led_set(unsigned char no, unsigned char value)
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
		exit(1);
	}
	if (value == LED_OFF){
		/* OFF */
		digitalWrite(no, HIGH);
	}else{	/* ON */
		digitalWrite(no, LOW);
	}
	return 0;
}

int led_init(void)
{
	int i;

        if (wiringpi_setup_flag == 0){
                wiringPiSetup();
                wiringpi_setup_flag = 1;
        }

	/* set initial value */
	pinMode (PIN_NO1, OUTPUT);
	pinMode (PIN_NO2, OUTPUT);
	pinMode (PIN_NO3, OUTPUT);
	/* light off */
	for (i=1; i<=3; i++){
		led_set(i, LED_OFF);
	}

	return 0;
}

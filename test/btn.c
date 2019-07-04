#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "press.h"
#include "btn.h"

volatile  int8_t btnRed;
volatile  int8_t btnGreen;
volatile  int8_t btnBlue;
volatile  int8_t btnYellow;

extern int wiringpi_setup_flag;

void myIntrRed(void){
	int8_t tmp;

	/* negative logic -> positive logic*/
	printf("btn:red callback\n");
	piLock(BTN_LOCK);
	tmp = btnRed;
	btnRed = (~digitalRead(RED)) & 0x1;
	piUnlock(BTN_LOCK);
	if (tmp == BTN_OFF && btnRed == BTN_ON) {
		press_set_auto_zero();
	}
}
void myIntrGreen(void){
	piLock(BTN_LOCK);
	btnGreen = (~digitalRead(GREEN)) & 0x1;
	piUnlock(BTN_LOCK);
}
void myIntrBlue(void){
	piLock(BTN_LOCK);
	btnBlue = (~digitalRead(BLUE)) & 0x1;
	piUnlock(BTN_LOCK);
}
void myIntrYellow(void){
	piLock(BTN_LOCK);
	btnYellow = (~digitalRead(YELLOW)) & 0x1;
	piUnlock(BTN_LOCK);
	if (btnYellow == BTN_ON) {
		/* system shutdown */
		system("sudo poweroff");
	}
}

int btn_init(void)
{
	if (wiringpi_setup_flag == 0){
		wiringPiSetup();
		wiringpi_setup_flag = 1;
	}

	/* read initial value */
	myIntrRed();
	myIntrGreen();
	myIntrBlue();
	myIntrYellow();

	wiringPiISR(RED, INT_EDGE_BOTH, myIntrRed);
	wiringPiISR(GREEN, INT_EDGE_BOTH, myIntrGreen);
	wiringPiISR(BLUE, INT_EDGE_BOTH, myIntrBlue);
	wiringPiISR(YELLOW, INT_EDGE_BOTH, myIntrYellow);

	return 0;
}

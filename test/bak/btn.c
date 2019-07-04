#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "btn.h"

volatile  int8_t btnRed;
volatile  int8_t btnGreen;
volatile  int8_t btnBlue;
volatile  int8_t btnYellow;

void myIntrRed(void){
	/* negative logic -> positive logic*/
	btnRed = (~digitalRead(RED)) & 0x1;
}
void myIntrGreen(void){
	btnGreen = (~digitalRead(GREEN)) & 0x1;
}
void myIntrBlue(void){
	btnBlue = (~digitalRead(BLUE)) & 0x1;
}
void myIntrYellow(void){
	btnYellow = (~digitalRead(YELLOW)) & 0x1;
}

int init_btn(void)
{
	wiringPiSetup();

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

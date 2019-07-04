#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wiringPi.h>
#include <sys/time.h>

#include "press.h"
#include "key.h"
#include "led.h"
#include "log.h"
#include "rec.h"
#include "khaen.h"
#include "btn.h"

extern int wiringpi_setup_flag;

static btn_onoff_t btn_onoff[BTN_NUM];

static void calib(void)
{
	press_set_auto_zero(PRESS_AUTO_DIFF_FORCE);
}

int unshi_mode = 0;
static void unshi_on(void)
{
	unshi_mode = 1;
	led_set(3, LED_ON);
}
static void unshi_off(void)
{
	unshi_mode = 0;
	led_set(3, LED_OFF);
}
static void recbtn(void)
{	
	log_prt("call recbtn\n");
	switch( rec_sts() ){
	case 0:
		rec_start();
		break;
	case 2:
		rec_save();
		break;
	default:
		/* no action */
		break;
	}
}
static void shtdwn(void)
{
	/* system shutdown */
	system("sudo poweroff");
}
static void restart(void)
{
	exit(0);
}
void vol_up(void)
{
//	khean_vol(1);
}
void vol_down(void)
{
//	khean_vol(-1);
}

const int col_list[BTN_NUM] ={ RED, BLUE, GREEN, YELLOW };
static int menu_sts = 0;
static menu_t menu[][BTN_NUM] = {
{ /* normal */
	{calib,		0},
	{recbtn,	0},
	{unshi_on,	1},
	{shtdwn,	0},
},
{ /* unshi + volume */
	{unshi_off,	0},
	{restart,	0},
	{NULL,		1},
	{NULL,		1},
},
};
void select_menu(int color)
{
	char c[256];
	menu_t *p;

	sprintf(c, "select_menu:color=%d, menu_sts=%d\n",
		color, menu_sts);
	//log_prt(c);
	p = &menu[menu_sts][color];
	if ( p->func != NULL){
		p->func();
	}
	menu_sts = p->n_sts;
}
static uint8_t get_btn(int color)
{
	/* negative logic -> positive logic*/
	return  (~digitalRead(color)) & 0x1;
}
static void check_btn(void)
{
	int i;
	btn_onoff_t *pb = btn_onoff;

	for (i=0; i < BTN_NUM; i++){
		if (pb->chg && pb->val){ /* off -> on */
		  /* call func */
		  select_menu(i);
		  pb->chg = 0;
		}
		pb++;
	}
}
static void dis_chatter(void)
{
	uint8_t i, onoff;
        static b_chatt_t ct[BTN_NUM];
        /* sts: 0:fix, 1:wait_antei */
        b_chatt_t *pct;
        static int notfirst = 0;

        if (notfirst){
		pct = ct;
		for (i=0; i< BTN_NUM; i++){
			onoff = get_btn(col_list[i]);
			if (pct->prev != onoff ){
				pct->prev = onoff;
                  		pct->num = 1;
                        	pct->sts = 1;
                	}else if (pct->sts == 1){
                        	/* wait antei , prev == now */
                        	pct->num++;
                        	if (pct->num >= BTN_CHATTERING_NUM){
                               	  /* new val */
                                  pct->num = 0;
                                  pct->sts = 0;
				  btn_onoff[i].val = pct->prev;
				  btn_onoff[i].chg = 1;
				}
			}
			pct++;
		
		}
	}else{ /* first(init) */
		for (i=0; i< BTN_NUM; i++){
			btn_onoff[i].val = ct[i].prev = get_btn(col_list[i]);
		}
		notfirst++;
	}
}
PI_THREAD (btn_loop)
{
	int i;
        while(1){
		for ( i=0; i < 10; i++){
                	dis_chatter();
                	usleep(BTN_INT_USEC);
		}
		check_btn();
        }
}
int btn_init(void)
{
	if (wiringpi_setup_flag == 0){
		wiringPiSetup();
		wiringpi_setup_flag = 1;
	}
	pullUpDnControl (RED, PUD_UP);
	pullUpDnControl (BLUE, PUD_UP);
	pullUpDnControl (GREEN, PUD_UP);
	pullUpDnControl (YELLOW, PUD_UP);

        /* create btn thread */
        int x;
        x = piThreadCreate (btn_loop) ;
        if (x != 0)
                log_prt("btnThread didn't startn");

        log_prt("end:btn_init\n");

	return 0;
}

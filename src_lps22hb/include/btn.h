/* btn.h */

#ifndef _BTN_H_
#define _BTN_H_

#define BTN_LOCK 2
#define BTN_ON	1
#define BTN_OFF	0
#define BTN_NUM	4

#define BTN_INT_USEC	10000
#define BTN_CHATTERING_NUM	3

typedef struct {
        uint8_t val;
        uint8_t chg;
} btn_onoff_t;

typedef struct {
        int8_t color_index;
	unsigned char sts;
	unsigned char num;
	unsigned char prev;
} b_chatt_t;

typedef void (*MENUPTR)(void);
typedef struct {
	MENUPTR func;
	int n_sts;
} menu_t;

//wPi No.
#define RED	21
#define BLUE	22
#define GREEN	23
#define YELLOW 	25

int btn_init(void);

#endif /* _BTN_H_ */

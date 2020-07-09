// MS5611 control pressure sensor
// press.c
// 2019.04.07

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "led.h"
#include "btn.h"
#include "press.h"
#include "monitor.h"
#include "log.h"
#include "key.h"
#include "rec.h"
#include "pcm.h"
//#include "cos_tbl.h"
#include "cmp.h"
#include "midi.h"

static char strbuf[512];
extern int wiringpi_setup_flag;
CAL_DATA_T cal_data;

volatile uint32_t press_val = 0; /* get measured pressure */
volatile uint32_t press_diff = 0;
volatile uint32_t press_diff_prev = 0;
volatile uint32_t press_std = 0;
volatile int    press_std_flag = 0; /* get standard pressure */
volatile uint16_t temp_val = 0; /* get measured temperature */

extern int unshi_mode;
extern int pcm_read_cnt;

static int cmdSPI(unsigned char addr);
int get_val(void);
static void ms5611_cal(CAL_DATA_T* cal_data, uint32_t d1, uint32_t d2,
		uint32_t** press, uint16_t** temp);
static int read_press_temp(uint32_t *press, uint16_t *temp);

void nano_slp(long nano_sec)
{
   	static struct timespec ts;
   	ts.tv_sec = 0;
   	ts.tv_nsec = nano_sec;

	clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
}

static int cmdSPI(unsigned char addr)
{
	unsigned char array;
	int ret;

	array = addr; 
	ret = wiringPiSPIDataRW (PRESS_CH, &array, 1);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}

	return 0;
}

static int readSPIm(unsigned char addr, unsigned char **data, int len)
{
	int ret;
	static unsigned char buf[16+1];

	if (len > 16){
		log_prt("error in readSPIm/n");
		return -1;
	}
	buf[0] = addr;
	ret = wiringPiSPIDataRW (PRESS_CH, buf, len+1);
	if (ret == -1){
		sprintf(strbuf, "%s:errno:%d\n", __func__, errno);
		//log_(strbuf);
		return 1;
	}
	*data = &(buf[1]);

	return 0;
} 

void press_set_auto_zero(int flag)
{
#if 1
	/* exist_key and not first */
	if (read_key() && press_std != 0 ){
		log_prt("read_key=0x%x:flag:%d, press_set_auto_zero() return\n",
			flag, read_key() );
		return;
	}
#endif
	piLock(PRESS_LOCK);
	press_std_flag = 1 | flag;
	piUnlock(PRESS_LOCK);
	led_set(LED_ZERO, LED_ON);
}
PI_THREAD (autoZero)
{
	while(1){
		delay(PRESS_AUTO_ZERO_SEC * 1000);
		press_set_auto_zero(0); /* no force */
	}
}

PI_THREAD (pressThr)
{
	int cnt = 0; 
	while(1){
		usleep(WAIT_PRESS_USEC); 
		get_val();
#if 1
		cnt %= 10000;
		if ( cnt == 0){
    			log_prt("cycle_10k:press:%d, temp:%d\n", 
				press_val, temp_val );
		}
		cnt+=2;
#endif
	}
}
int press_init(void)
{
	int fd;
	uint8_t *data;
	//uint16_t d0, d7;

        led_init();
        btn_init();
        key_init();
        pcm_init();
        rec_init();
        monitor_init();
	//audio compressor
        //cmp_init(20, 100, 32768/8*5, 30);
	//cmp_init();
	midi_init();

        if (wiringpi_setup_flag == 0){
                wiringPiSetup();
                wiringpi_setup_flag++;
        }
	fd =  wiringPiSPISetup (PRESS_CH, 10000000) ; //10MHz
	if (fd == -1){
		sprintf(strbuf, "wiringPiSPISetup:errno:%d\n", errno);
		log_prt(strbuf);
		return 1;
	}else{
		sprintf(strbuf, "wiringPiSPISetup:fd:%d\n", fd);
		log_prt(strbuf);
	}

	/* RESET */
	cmdSPI(MS_RESET);
	usleep(5000);

	/* READ_PROM */
	CAL_DATA_T* g = &cal_data;
#if 0
	readSPIm(MS_PROM_0, &data, 2);
	d0 = data[0]<<8 | data[1];
#endif
	readSPIm(MS_PROM_2, &data, 2);
	g->c1 = data[0]<<8 | data[1];
	readSPIm(MS_PROM_4, &data, 2);
	g->c2 = data[0]<<8 | data[1];
	readSPIm(MS_PROM_6, &data, 2);
	g->c3 = data[0]<<8 | data[1];
	readSPIm(MS_PROM_8, &data, 2);
	g->c4 = data[0]<<8 | data[1];
	readSPIm(MS_PROM_A, &data, 2);
	g->c5 = data[0]<<8 | data[1];
	readSPIm(MS_PROM_C, &data, 2);
	g->c6 = data[0]<<8 | data[1];
#if 0
	readSPIm(MS_PROM_E, &data, 2);
	d7 = data[0]<<8 | data[1];
	log_prt("%02x %02x,%02x,%02x,%02x,%02x,%02x %02x\n",
		d0, g->c1, g->c2, g->c3, g->c4, g->c5, g->c6, d7);
	log_prt("init done...\n");
#endif
	press_set_auto_zero(PRESS_AUTO_DIFF_FORCE);

        /* autozero create timer thread */
        int x;
        x = piThreadCreate (autoZero) ;
        if (x != 0){
                log_prt("timerThread didn't startn");
                exit(1);
        }
#if 1 // if 0 == get_val() is called from key.c
        x = piThreadCreate (pressThr) ;
        if (x != 0){
                log_prt("timerThread didn't startn");
                exit(1);
        }
#endif
	usleep(10000);
	log_prt("end:press_init\n");
	return 0;
}

static int read_press_temp(uint32_t *ppress, uint16_t *ptemp)
{
        uint8_t *data;
	uint32_t d1;
	static uint32_t d2;
	uint32_t temp_cnt = 0;
#if 1
        cmdSPI(MS_C_D1_512); /* D1:pressure */
	//usleep(1100);
	nano_slp(1100000); //nano_sec
#else
        cmdSPI(MS_C_D1_256); /* D1:pressure */
	//usleep(600);
	nano_slp(600000); //nano_sec
#endif
        readSPIm(MS_ADC, &data, 3); /* read value */
	d1 = data[0]<<16 | data[1]<<8 | data[2];

	temp_cnt %= 512;
	if ( 0 == temp_cnt ){
        	cmdSPI(MS_C_D2_256); /* D2:temperature */
		usleep(600);
        	readSPIm(MS_ADC, &data, 3); /* read value */
		d2 = data[0]<<16 | data[1]<<8 | data[2];
	}
	temp_cnt++;

	ms5611_cal(&cal_data, d1, d2, &ppress, &ptemp);

	return 0;
}

/* read diff value */
uint32_t press_read(void)
{
	uint32_t tmp;

	piLock(PRESS_LOCK);
        tmp = press_diff;
	piUnlock(PRESS_LOCK);
	return tmp;
}
uint16_t temp_read(void)
{
	uint16_t tmp;

	piLock(PRESS_LOCK);
        tmp = temp_val;
	piUnlock(PRESS_LOCK);
	return tmp;
}
static uint32_t press_hist(uint32_t mea, uint32_t std)
{
        press_diff_prev = press_diff;
        /* reset pcm read counter */
        pcm_read_cnt = 0;

	/* breathe out (exhale) */
	if ( mea > std){
	  return mea - std;
	}
	/* breathe in (inhale) */
	return std - mea;
}
int get_val(void) /* get value from device */
{
	uint32_t	press; //pressure value(hPa)  1017*100
	uint16_t	temp = 0;  // temperature
//	static int cnt = 0;

	read_press_temp(&press, &temp);
#if 0
	if ( press > 0xf0000000 ){
		log_prt("reverse press val\n");
		press_std_flag = 2; /* force reset diff */
		usleep(5000);
		read_press_temp(&press, &temp);
	}
#endif

#if 0
	cnt++;
	cnt %= 1000;
	if (cnt == 0){
		log_prt("press:1000 times\n");
	}
#endif
	temp_val = temp; 
	//printf("press:0x%x\n", press); /* debug print */
	/* set standard value and diff */
	piLock(PRESS_LOCK);
	/* renew standard value */
	if ( press_std_flag ){
		//printf("set standard, press:0x%x\n", press);
		//if ( (PRESS_AUTO_DIFF_MAX < abs((int)press - press_std))
		if ( (PRESS_AUTO_DIFF_MAX > abs((int)press - press_std))
			|| (press_std_flag & PRESS_AUTO_DIFF_FORCE) ){
			/* reset diff */
			log_prt("reset diff:0x%x\n", press);
			press_std = press;
        		press_diff = 0;
		}else{
			/* discard */
			press_diff = press_hist(press, press_std);
		}
		press_std_flag = 0;
		led_set(LED_ZERO, LED_OFF);
	}else{
		press_diff = press_hist(press, press_std);
	}
        press_val = press;
	piUnlock(PRESS_LOCK);

	return 0;
}

/*
 * Read and compensate a temperature and pressure from the MS5611.
 *
 * `cal_data` is previously read calibration data.
 * `temperature` and `pressure` are written to.
 *
 * `temperature` is in centidegrees Celcius,
 * `pressure` is in Pascals.
 */
static void ms5611_cal(CAL_DATA_T* cal_data,
	uint32_t d1, uint32_t d2, uint32_t** press, uint16_t **temp)
{
    int64_t off, sens, dt;
    int32_t temperature, pressure;

    dt = (int64_t)d2 - (int64_t)cal_data->c5 * (1<<8);
    temperature = 2000 + (dt * (int64_t)cal_data->c6) / (1<<23);
    
    off = (int64_t)cal_data->c2 * (1<<16) + \
          ((int64_t)cal_data->c4 * dt) / (1<<7);

    sens = (int64_t)cal_data->c1 * (1<<15) + \
           ((int64_t)cal_data->c3 * dt) / (1<<8);

    pressure = ((d1 * sens) / (1<<21) - off) / (1<<15);
    
    /*update the global variables with the pressure and temperature information*/
    **press = pressure;
    **temp = temperature;

}

double dyn_press_diff(void)
{
        /* press sensor 500/sec  = 2msec */
        /* PCM 48sample/msec */
        if (pcm_read_cnt >= PCM_FULL_CNT) return press_diff;
        return press_diff_prev + ((double)press_diff - press_diff_prev)
                * ((double)pcm_read_cnt) / PCM_FULL_CNT;
}

int press_amp_factor(double* pfac)
{
	double tmp;

	if (unshi_mode){
		*pfac = PRESS_UNSHI_FAC;
		return 0;
	}
	piLock(PRESS_LOCK);
	/* 1 hPa = 100 count, factor max is 1024 */
	tmp = dyn_press_diff() * 1024 / PRESS_MAX;
	piUnlock(PRESS_LOCK);

	/* human ear adjustment */
#if 1
	/* offset curve */
	if (tmp < PRESS_OFFSET){
		tmp = 0;
	} else if (tmp <= 1024-PRESS_OFFSET ){
		tmp = ((tmp-PRESS_OFFSET)*1024)/(1024 - PRESS_OFFSET);
	} else tmp = 1024;
#else
	/* cosine table */
	if ( tmp < 0){
		tmp = 0;
	} else if ( tmp >= 1024){
		tmp = 1024;
	} else if (tmp >= 0 && tmp < 1024){
		tmp = cos_tbl[(int)tmp];
	} 
#endif

	*pfac = tmp;
	return 0;
}

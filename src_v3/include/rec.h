/* rec.h */
#include <stdint.h>

#ifndef _REC_H_ 
#define _REC_H_

#define LEN_SIZE 4 /* byte */
#define MAX_STR 256 /* contain byte size */
#define REC_LIMIT_NUM       20
#define REC_DIR         "/home/pi/rec/"
#define REC_NO_FNAME    "./rec_no_"
#define REC_TIME_MIN	2	/* recoard time in min */


void rec_init(void);
void rec_start(void);
void rec_write(int16_t val);
void rec_save(void);
void rec_free(void);
int rec_sts(void);

#endif /* _REC_H_ */

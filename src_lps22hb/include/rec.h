/* rec.h */
#include <stdint.h>

#ifndef _REC_H_ 
#define _REC_H_

#define REC_SIZE (48000*2*180) /* bytes in 3min */
#define LEN_SIZE 4 /* byte */
#define MAX_STR 256 /* contain byte size */

void rec_init(void);
void rec_start(void);
void rec_write(int16_t val);
void rec_save(void);
void rec_free(void);
int rec_sts(void);

#endif /* _REC_H_ */

/* cmp.h */

#ifndef _COMPRESS_H_
#define _COMPRESS_H_

#define SAMP_RATE       48000
#define C_NORMAL        0
#define C_ATTACK        1
#define C_COMP          2
#define C_REL           3
#define BUF_NUM         256


extern void cmp_init(int att_msec, int rel_msec, int thr, int cmp_per);
int compress(int val);

#endif /* _COMPRESS_H_ */

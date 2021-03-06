/* pcm.h */

#include <sys/types.h>
#include <stdint.h>

#ifndef _PCM_H_
#define _PCM_H_

#define HEADER_SIZE 0x2c
#define S_ON	1
#define S_OFF	0
#define S_AD	0 /* sts: attack & decay */
#define S_SU	1 /* sastain */
#define S_RE	2 /* release */
#define WAV_DIR "/home/pi/khaen/wav"	/* wav file folder */
#define WAV_S_DIR "/home/pi/o_wav"	/* output wav file folder */
#define STR_MAX 200
#define PERIOD_SIZE 2048
#define PCM_NUM 16
#define PCM_LOCK 3
#define SAMPLES_PER_MSEC	48
#define PEAK_SMP 250
#define EXPAND_SUS	150

typedef struct {
	const int w_idx; /* wav file name index */
	short *ad_ptr; /* pcm data pointer */
	short *ptr;
	int sts; /* status */
	int ad_cnt; /* pcm data num(count) */
	int cnt;
	int re_cnt;
	int re_dcnt; /* dynamic count */
	int loc; /* data location(offset) */
	int re_1stloc; /* 1st data location for release(offset) */
	int fac_rel; /* factor at release */
	int prev_on; /* key previous state */
	const int cross;   /* enable cross fade */
	const int amp;   /* amplitude(100%) */
	const double f_tgt; /* target freq */
} wav_pcm_t;

typedef struct {
	const char *ad_fn;
	const char *s_fn;
	const float freq;
} wav_fn;

int pcm_init(void);
int pcm_free(void);
int pcm_read(int called, int count);
void pcm_set_key(uint16_t key);
int pre_p_proc(void);
int write_file( const char *out_fname, short* ptr, unsigned int len);


#endif /* _PCM_H_ */

/* p_file.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <sndfile.h>
#include "log.h"
#include "press.h"
#include "pcm.h"

wav_pcm_t *mix_wav;
int pcm_id = 0;

#include "pcm_data.h"

int read_file2( const char *in_fname, short** pptr, unsigned int* len)
/* Note:len is samples */
{
	SNDFILE		*sfp;
	SF_INFO		sfinfo;
	int		cnt;

	sfinfo.format = 0;

	sfp = sf_open(in_fname, SFM_READ, &sfinfo);
	if (sfp == NULL){
		log_prt("sf_open(%s) err\n", in_fname);
		return -1;
	}
#if 0
	printf("cnt:%lld, rate:%d, ch:%d, fmt:0x%x\n",
	  sfinfo.frames, sfinfo.samplerate, sfinfo.channels,
	  sfinfo.format);
#endif

	*pptr = calloc( sfinfo.frames, sizeof(short) );
	if  (*pptr == NULL){
        	log_prt("pcm.c calloc err\n");
		sf_close(sfp);
        	return -2;
    	}
	cnt = sf_read_short(sfp, *pptr, sfinfo.frames);
	if  (cnt != sfinfo.frames){
        	log_prt("sfinfo.frames:%d vs cnt:%d\n", sfinfo.frames, cnt);
		sf_close(sfp);
        	return -3;
    	}
	//log_prt("read %d samples\n", cnt);
	*len = sfinfo.frames;
	sf_close(sfp);
	//log_prt("1st pcm:%02x\n", *(*pptr) );
	return 0;
}

int pcm_free(void)
{
	int i;
	wav_pcm_t *p;

	//log_prt("pcm_free\n");
	for (i = 0; i < PCM_NUM; i++){
		p = &mix_wav[i];
		free(p->ptr);
		free(p->ad_ptr);
	}
	return 0;
}

int pcm_init(void)
{
	int i;
	int ret = 0;
	unsigned int len;
	char path_wav[STR_MAX + 1];

	wav_pcm_t *p;

	log_prt("key code(Am, Fm, etc):%d\n", pcm_id);
	mix_wav = pcm_data[pcm_id];

	log_prt("pcm_init:pcm_id=%d\n", pcm_id);
	for (i = 0; i < PCM_NUM; i++){
		//printf("pcm_init:i=%d\n", i);
		p = &mix_wav[i];
	        sprintf(path_wav, "%s%d/%s.wav",
		  WAV_S_DIR, pcm_id, pcm_fn[i].s_fn);
		ret = read_file2(path_wav, &p->ptr, &len );
		if (0 > ret ){
			log_prt("s_fn:error:%d, %d\n", i, ret );
			exit(1);
		}
		p->cnt = len;

	        sprintf(path_wav, "%s%d/%s.wav",
		  WAV_S_DIR, pcm_id, pcm_fn[i].ad_fn);
		ret = read_file2(path_wav, &p->ad_ptr, &len );
		if (0 > ret ){
			log_prt("ad_fn:error:%d, %d\n", i, ret);
			exit(-1);
		}
		p->ad_cnt = len;
		/* Relase data are produced from sustain data */
                /* 440Hz sample in std time, low freq => long */
                p->re_cnt = SAMPLES_PER_MSEC
                             *( 50 + 100*(440.0/abs(p->f_tgt)) );
                //             *( 100 + 200*(440.0/(p->f_tgt)) );

	}
	log_prt("pcm:init end\n");
	return 0;
}


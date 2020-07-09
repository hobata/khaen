#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sched.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>
#include <libconfig.h>
#include "press.h"
#include "conf.h"
#include "khaen.h"
#include "touch.h"
#include "pcm.h"

#define MAXFD 64
#define MAX_KEY_CODE 1

extern unsigned int period_time;
extern unsigned int buffer_time;
extern unsigned int rec_num;
extern unsigned int max_log;
extern unsigned int ring_buf_enb;
extern unsigned int stdout_disable;
extern unsigned int t_func_no;
extern unsigned int p_func_no;
extern int pcm_id;

void help(void);
extern void get_cmd_opt(int argc, char *argv[]);

static void setprio(int prio, int sched)
{
   	struct sched_param param;
   	// Set realtime priority for this thread
   	param.sched_priority = prio;
   	if (sched_setscheduler(0, sched, &param) < 0)
   		perror("sched_setscheduler");
}

void destroy(void)
{
	conf_destroy();
}
void proc_pre_proc(void)
{
	if (pre_t_proc()){
		destroy();
		exit(0);
	}
	if (pre_p_proc()){
		destroy();
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;

	conf_init();
	get_cmd_opt(argc, argv);
	proc_pre_proc();
#if 0
	/* Now lock all current and future pages 
   	   from preventing of being paged */
   	if (mlockall(MCL_CURRENT | MCL_FUTURE)){
   		perror("mlockall failed:");
		exit(1);
	}
#endif
	setprio(sched_get_priority_max(SCHED_RR), SCHED_RR);

    ret =  khean();
    destroy();
    return ret;
}

void get_cmd_opt(int argc, char *argv[])
{
	int opt;
	int cnt = 0;

	while ((opt = getopt(argc, argv, "P:t:p:b:r:l:Rsk:")) != -1){
		cnt++;
		switch(opt){
		case 'p':
			period_time = atoi(optarg);
			if (0==period_time){
				period_time = 1000;
			}
			break;
		case 'b':
			buffer_time = atoi(optarg);
			if (0==buffer_time){
				buffer_time = 2000;
			}
			break;
		case 'r':
			rec_num = 48000*60*atoi(optarg);
			if (0==rec_num){
				rec_num = 48000*60*2; /* 2min */
			}
			break;
		case 'l':
			max_log = atoi(optarg)*1024;
			if (0==max_log){
				max_log = 32768;
			}
			break;
		case 'k':
			pcm_id = atoi(optarg);
			if (pcm_id > MAX_KEY_CODE){
				pcm_id = MAX_KEY_CODE;
			}
			break;
		case 'R':
			ring_buf_enb = 1;
			break;
		case 's':
			stdout_disable = 1;
			break;
		case 't':
			/* not zero */
			t_func_no = atoi(optarg);
			break;
		case 'P':
			/* not zero */
			p_func_no = atoi(optarg);
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [-b usec] [-p usec] [-r min] [-l kByte] [-R] [-s] [-t func_no] [-P func_no]\n", argv[0]);
			help();
			exit(EXIT_FAILURE);
		}
	}
	if (0==cnt){
		fprintf(stderr, "Usage: %s [-b usec] [-p usec] [-r min] [-l kByte] [-R] [-s] [-t func_no] [-P func_no]\n", argv[0]);
		help();
	}
}

void help(void)
{
	printf("\tb:alsa_buffer(usec)[2000]\n");
	printf("\tp:alsa_period(usec)[1000]\n");
	printf("\tr:record_duration(min)[2]\n");
	printf("\tl:log_size(kByte)[32]\n");
	printf("\tR:log_ring_buffer_enable\n");
	printf("\ts:stdout_disable\n");
	printf("\tt:touch_sensor_function_no(not_zero)\n");
}

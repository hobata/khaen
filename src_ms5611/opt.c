#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if 0
extern unsigned int period_time;
extern unsigned int buffer_time;
extern unsigned int rec_size;
extern unsigned int max_log;
extern unsigned int ring_buf_enb;
extern unsigned int stdout_disable;
#else
unsigned int period_time;
unsigned int buffer_time;
unsigned int rec_size;
unsigned int max_log;
unsigned int ring_buf_enb;
unsigned int stdout_disable;
#endif

void help(void);
void get_cmd_opt(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	get_cmd_opt(argc, argv);
	return 0;
}
void get_cmd_opt(int argc, char *argv[])
{
	int opt;
	int cnt = 0;

	while ((opt = getopt(argc, argv, "p:b:r:l:Rs")) != -1){
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
			rec_size = 48000*60*atoi(optarg);
			if (0==rec_size){
				rec_size = 48000*60*2;
			}
			break;
		case 'l':
			max_log = atoi(optarg)*1024;
			if (0==max_log){
				max_log = 32768;
			}
			break;
		case 'R':
			ring_buf_enb = 1;
			break;
		case 's':
			stdout_disable = 1;
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [-b usec] [-p usec] [-r min] [-l kByte] [-R] [-s]\n", argv[0]);
			help();
			exit(EXIT_FAILURE);
		}
	}
	if (0==cnt){
		fprintf(stderr, "Usage: %s [-b usec] [-p usec] [-r min] [-l kByte] [-R] [-s]\n", argv[0]);
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
}

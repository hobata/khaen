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

extern unsigned int p_func_no;
extern int pre_p_proc();

int main(int argc, char *argv[])
{
	int ret = 0;
	
	printf("start make wav file\n");
	p_func_no = 1;
	pre_p_proc();
	printf("finished..\n");

	return ret;
}

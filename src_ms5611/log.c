/* log.c */
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"
#include "common.h"

int ring_buf_enb = 0;
int stdout_disable = 0;
int max_log = 32768;

char lstr[PRE_MAX_LOG]; //last element is NULL(Termination)
char* ladr = lstr;

void log_prt(const char *fmt, ...)
{
        char lbuf[256] ={'\0'};
	char *pbuf =lbuf;
        time_t  now = time(NULL);
        struct  tm *ts;
	int n, ntmp;
	const char *lbuf_end = lstr + max_log - 1;

        ts = localtime(&now);
        ntmp = sprintf(pbuf, "[%02d:%02d:%02d]",
                ts->tm_hour, ts->tm_min, ts->tm_sec);
	if (ntmp < 1) ntmp = 0;
	n = ntmp;
	pbuf += n;

        va_list ap;
        va_start(ap, fmt);
        ntmp = vsprintf(pbuf, fmt, ap);
	if (ntmp < 1) ntmp = 0;
	n += ntmp;
        va_end(ap);
	if (0 == stdout_disable){
		printf("%s", lbuf);
	}
	if (lbuf_end < ladr + n){
		if (0==ring_buf_enb){
			return; //no ring buffer
		}else{
			ladr = lstr; //ring buffer overwrite
		}
	}
	memcpy(ladr, lbuf, n);
	ladr += n;
}
void make_log_file(void)
{
        FILE *fp;
        char fname[MAX_STR];
        time_t  now = time(NULL);
        struct  tm *ts;

        ts = localtime(&now);
        sprintf(fname, "%s%04d%02d%02d_%02d%02d%02d.log",
		LOG_DIR,
                ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
                ts->tm_hour, ts->tm_min, ts->tm_sec);
        fp = fopen(fname, "w");
        if (fp == NULL){
                printf("rec:fopen err\n");
                exit(-1);
        }
        if ( !fwrite(lstr, 1, strlen(lstr), fp)){
                printf("rec:fwrite err\n");
                exit(-1);
        }
        fflush(fp);
        fclose(fp);
	del_file(LOG_DIR, LIMIT_NUM);
}


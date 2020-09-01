/* log.h */

#ifndef _LOG_H_
#define _LOG_H_
#include <stdarg.h>

#define MAX_STR 256
#define LIMIT_NUM       10
#define LOG_DIR         "/home/pi/log/"

#define log_str	log_prt

void log_prt(const char *fmt, ...);
void make_log_file(void);

#endif /* _LOG_H_ */


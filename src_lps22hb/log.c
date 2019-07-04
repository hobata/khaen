/* log.c */
#include <stdio.h>
#include <syslog.h>

extern int g_daemon;

int log_(const char *fmt)
{
  if (g_daemon){
    syslog(LOG_USER | LOG_NOTICE, fmt);
  }else{
    printf(fmt);
  }
  return 0;
}
int log_str(const char *fmt, const char *prm)
{
  if (g_daemon){
    syslog(LOG_USER | LOG_NOTICE, fmt, prm);
  }else{
    printf(fmt, prm);
  }
  return 0;
}


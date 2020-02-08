/* conf.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libconfig.h>
#include "conf.h"
#include "log.h"

static int conf_enable = 0;

static config_t cfg;
static char buff[256];

extern uint16_t mask_drone;
extern int pcm_id;
int rec_no;

void conf_init(void)
{
  int val;
  const char *str;

  config_init(&cfg);
#if 0
  config_set_options(&cfg,
                     (CONFIG_OPTION_FSYNC
                      | CONFIG_OPTION_SEMICOLON_SEPARATORS
                      | CONFIG_OPTION_COLON_ASSIGNMENT_FOR_GROUPS
                      | CONFIG_OPTION_OPEN_BRACE_ON_SEPARATE_LINE));
#endif
  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, CFG_FILE))
  {
    sprintf(buff, "%s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    log_prt("%s", buff);
    config_destroy(&cfg);
    conf_enable = 0;
    return;
  }

  /* Get the store name. */
  if(config_lookup_string(&cfg, "name", &str)){
    sprintf(buff, "config name: %s\n", str);
    log_prt("%s", buff);
  }else{
    sprintf(buff, "No 'name' setting in configuration file.\n");
    log_prt("%s", buff);
  }

  /* get vales */
  config_lookup_int(&cfg, "khaen.key", &val);
  pcm_id = val;

  config_lookup_int(&cfg, "khaen.drone", &val);
  mask_drone = (uint16_t)val;

  config_lookup_int(&cfg, "khaen.rec_no", &val);
  rec_no = val;

  sprintf(buff, "read config:%d:%d:%d\n",
	pcm_id, mask_drone, rec_no);
  log_prt("%s", buff);

  conf_enable = 1;
}
void conf_destroy(void)
{
  if (!conf_enable) return;
  config_destroy(&cfg);
}
void conf_key(int val)
{
  config_setting_t *setting;
  if (!conf_enable) return;
  setting = config_lookup(&cfg, "khaen.key");
  config_setting_set_int(setting, val);
  conf_write();
}
void conf_drone(int val)
{
  config_setting_t *setting;
  if (!conf_enable) return;
  setting = config_lookup(&cfg, "khaen.drone");
  config_setting_set_int(setting, val);
  conf_write();
}
void conf_rec(int val)
{
  config_setting_t *setting;
  if (!conf_enable) return;
  setting = config_lookup(&cfg, "khaen.rec_no");
  config_setting_set_int(setting, val);
  conf_write();
}
void conf_write(void)
{
  /* Write out the updated configuration. */
  if(! config_write_file(&cfg, CFG_FILE))
  {
    sprintf(buff, "Error while writing file.\n");
    log_prt("%s", buff);
    config_destroy(&cfg);
    conf_enable = 0;
  }
}

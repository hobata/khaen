/* conf.h */

#define CFG_FILE	"/home/pi/khaen/src/khaen.cfg"

void conf_init(void);
void conf_destroy(void);
void conf_key(int val);
void conf_drone(int val);
void conf_rec(int val);
void conf_write(void);

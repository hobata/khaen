#!/bin/sh
# sudo apt-get install liasound2-dev

#gcc main.c btn_test.c btn.c -lasound -lwiringPi -lm -o btn_test 
gcc main.c key_test.c key.c -lasound -lwiringPi -lm -o key_test 
#gcc main.c led_test.c led.c -lasound -lwiringPi -lm -o led_test 
#gcc main.c btn.c press_test.c press.c led.c -lasound -lwiringPi -lm -o press_test 

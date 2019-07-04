#!/bin/sh
# sudo apt-get install liasound2-dev

gcc btn_test.c btn.c -lasound -lwiringPi -lm -o btn_test 
gcc key_test.c key.c -lasound -lwiringPi -lm -o key_test 
gcc led_test.c led.c -lasound -lwiringPi -lm -o led_test 
gcc press_test.c press.c -lasound -lwiringPi -lm -o press_test 

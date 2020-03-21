#!/bin/sh

sudo pi
mkdir /home/pi/rec
mkdir /home/pi/log
mkdir /home/pi/o_wav0
mkdir /home/pi/o_wav1
mkdir /home/pi/o_wav2
mkdir /home/pi/o_wav3
mkdir /home/pi/o_wav4
mkdir /home/pi/o_wav5
mkdir /home/pi/o_wav6
cd ~/khaen/src
./khaen -P 1

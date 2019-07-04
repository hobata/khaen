#!/bin/sh
# sudo apt install libpulse-dev

gcc pacat-simple.c -lpulse -lpulse-simple -o pa


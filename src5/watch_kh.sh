#!/bin/bash

sudo /opt/vc/bin/tvservice -o

# "ip exist -> not start khaen"	
ex=$(ip link | grep BROADCAST | wc -l)
#echo $ex

if test "$ex" == "1"; then
	#echo "ip exist -> not start khaen"	
	exit
fi

while [ 1 -eq 1 ] ; do
ex=$(ps -e | grep khaen | head -1)
#echo "$ex"

if test "$ex" == ""; then
	#echo "no process"
	cd /home/pi/khaen/src 
	./khaen -s &
#else
	#echo "exist procdess"
fi

sleep 1

# check CPU usage
var=$(ps au -C khaen | grep -v grep | grep ./khaen | awk '{print $3}')
#echo $var
thr=5.0

result=`echo "$var > $thr" | bc`
if [ $result -eq 1 ]; then
    echo "OK:$var is large."
else
    echo "NG:$thr is large."
    #kill process
    pkill khaen
fi

#sleep 1

done

#!/bin/sh

`ps -e | grep watch_kh | head -1 | awk '{print "sudo kill ", $1}'`
`ps -e | grep khaen | head -1 | awk '{print "sudo kill ", $1}'`

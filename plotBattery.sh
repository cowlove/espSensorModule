#!/bin/bash

ssh rp1 grep -a 6F2C screen.mqtt.log | grep LIPOBATTERY | rematch 'LIPOBATTERY2=([0-9.]+)' | gnuplot -e "p '-' u (\$0/12):1 w l; pause 111"



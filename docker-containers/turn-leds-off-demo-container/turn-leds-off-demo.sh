#! /bin/sh

for led in /sys/class/leds/*
do
	if [ -f "${led}/trigger" ]
	then
		echo "none" > "${led}/trigger"
	fi
done

#! /bin/sh

while true
do
	for trigger in timer default-on heartbeat none
	do

		for led in /sys/class/leds/*
		do
			if [ -f "${led}/trigger" ]
			then
				echo "${trigger}" > "${led}/trigger"
			fi
		done
		sleep 15
	done
done

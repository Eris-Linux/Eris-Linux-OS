#!/bin/sh

if mount | grep media > /dev/null
then
	# Another device is already mounted.
	exit 0
fi

mount "${DEVNAME}" /media  || exit 1

for tk in /media/*.token
do
	if [ -f "${tk}" ]
	then
		cp "${tk}" /etc/eris-linux/token
		break
	fi
done

umount /media

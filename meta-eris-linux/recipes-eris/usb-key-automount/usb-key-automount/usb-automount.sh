#!/bin/sh

if mount | grep media > /dev/null
then
	# Another device is already mounted.
	exit 0
fi

mount "${DEVNAME}" /media  || exit 1

model="$(cat /usr/share/eris-linux/system-model)
if [ "${model}" = "" ]; then exit 1; fi

for f in $(ls -R /media/eris-linux_${model}*.tar.bz2)
do
	if [ -f "${f}" ]
	then
		mkdir -p /tmp/direct-update
		cp "${f}" /tmp/direct-update/image
		break
	fi
done

umount /media

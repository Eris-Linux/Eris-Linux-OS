#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-only
#

### BEGIN INIT INFO
# Provides:          bootmisc
# Required-Start:    $local_fs mountvirtfs
# Required-Stop:     $local_fs
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Misc and other.
### END INIT INFO

TIMESTAMP_FILE=/etc/timestamp

. /etc/default/rcS
[ -f /etc/default/timestamp ] && . /etc/default/timestamp
#
# Put a nologin file in /etc to prevent people from logging in before
# system startup is complete.
#
if test "$DELAYLOGIN" = yes
then
  echo "System bootup in progress - please wait" > /etc/nologin
  cp /etc/nologin /etc/nologin.boot
fi

#
# Set pseudo-terminal access permissions.
#
if test -c /dev/ttyp0
then
	chmod 666 /dev/tty[p-za-e][0-9a-f]
	chown root:tty /dev/tty[p-za-e][0-9a-f]
fi

#
# Apply /proc settings if defined
#
SYSCTL_CONF="/etc/sysctl.conf"
if [ -f "${SYSCTL_CONF}" ]
then
	if [ -x "/sbin/sysctl" ]
	then
		# busybox sysctl does not support -q
		VERBOSE_REDIR="1>/dev/null"
		if [ "${VERBOSE}" != "no" ]; then
			VERBOSE_REDIR="1>&1"
		fi
		eval /sbin/sysctl -p "${SYSCTL_CONF}" $VERBOSE_REDIR
	else
		echo "To have ${SYSCTL_CONF} applied during boot, install package <procps>."
	fi
fi

#
# Update /etc/motd.
#
if test "$EDITMOTD" != no
then
	uname -a > /etc/motd.tmp
	sed 1d /etc/motd >> /etc/motd.tmp
	mv /etc/motd.tmp /etc/motd
fi

: exit 0

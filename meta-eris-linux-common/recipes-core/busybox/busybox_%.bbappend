#
# `busybox_%.bbappend`
#
# (c) 2025-2026 Logilin.
#

# Add some tasklets:
#  - chrt
#  - taskset
#  - httpd
#  - ntpd
#  - i2ctools
#  - tune2fs

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://fragment.cfg"


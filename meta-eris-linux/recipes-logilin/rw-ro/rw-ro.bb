#
# `rw-ro.bb`
#
# Logilin 2024
#

SUMMARY = "Shell scripts to remount root filesystem in read-write or read-oonly modes."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

do_install() {
	install -d ${D}${base_sbindir}
	echo '#!/bin/sh' > ${D}${base_sbindir}/rw
	echo 'mount / -o remount,rw' > ${D}${base_sbindir}/rw
	chmod 0755 ${D}${base_sbindir}/rw
	echo '#!/bin/sh' > ${D}${base_sbindir}/ro
	echo 'mount / -o remount,ro' > ${D}${base_sbindir}/ro
	chmod 0755 ${D}${base_sbindir}/ro
}

FILES:${PN} += "${base_sbindir}/rw"
FILES:${PN} += "${base_sbindir}/ro"

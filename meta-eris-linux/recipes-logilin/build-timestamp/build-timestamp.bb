#
# `build-timastamp`
#
# Logilin 2024
#

SUMMARY = "Install a file containing the build timestamp."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

BUILD_TIMETAMP = "${@time.strftime('%Y%m%d-%H%M%S', time.gmtime())}"

do_install() {
	install -d ${D}${sysconfdir}
	echo "Build Timestamp: ${BUILD_TIMETAMP}" > ${D}${sysconfdir}/build-timestamp
}

do_install[nostamp] = "1"

FILES:${PN} += "${sysconfdir}/build-timestamp"

SUMMARY = "Eris Linux NTP service"
LICENSE = "LGPL-2.1-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/${LICENSE};md5=1a6d268fd218675ffea8be556788b780"

SRC_URI += "file://${BPN}"
SRC_URI += "file://start-${BPN}"
SRC_URI += "file://${BPN}.service"

inherit update-rc.d
INITSCRIPT_NAME = "start-${BPN}"
INITSCRIPT_PARAMS = "start 10 5 ."

inherit systemd
SYSTEMD_SERVICE:${PN} += "${BPN}.service"

do_install() {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/start-${BPN} ${D}${sysconfdir}/init.d/

	install -d ${D}/${systemd_system_unitdir}
	install -m 0644 ${WORKDIR}/${BPN}.service ${D}/${systemd_system_unitdir}/

	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/${BPN} ${D}${sbindir}/
}

FILES:${PN} += "${systemd_system_unitdir}/${BPN}.service"

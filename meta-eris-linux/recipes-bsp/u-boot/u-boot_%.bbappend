
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://eris-splashscreen.bmp"

do_install:append() {
	install -d ${d}/boot
	install -m 0644 ${WORKDIR}/eris-splashscreen.bmp  ${D}/boot/
}

do_deploy:append() {
	install -d ${DEPLOYDIR}
	install -m 0644 ${WORKDIR}/eris-splashscreen.bmp  ${DEPLOYDIR}/
}


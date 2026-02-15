
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://eris-splashscreen-01.bmp"
SRC_URI += "file://eris-splashscreen-02.bmp"

do_install:append() {
	install -d ${d}/boot
	install -m 0644 ${WORKDIR}/eris-splashscreen-02.bmp  ${D}/boot/
	install -m 0644 ${WORKDIR}/eris-splashscreen-01.bmp  ${D}/boot/
}

do_deploy:append() {
	install -d ${DEPLOYDIR}
	install -m 0644 ${WORKDIR}/eris-splashscreen-01.bmp  ${DEPLOYDIR}/
	install -m 0644 ${WORKDIR}/eris-splashscreen-02.bmp  ${DEPLOYDIR}/
}


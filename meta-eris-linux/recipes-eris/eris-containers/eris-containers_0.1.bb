
SUMMARY = "Eris containers execution scripts"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"


SRC_URI += "file://${BPN}"
SRC_URI += "file://${BPN}-early"
SRC_URI += "file://${BPN}-network"

inherit update-rc.d

PACKAGES =+ "${PN}-early ${PN}-network"

INITSCRIPT_PACKAGES = "${PN}-early ${PN}-network"

RDEPENDS:${PN} += "${PN}-early ${PN}-network"

INITSCRIPT_NAME:${PN}-early = "${BPN}-early"
INITSCRIPT_PARAMS:${PN}-early = "start 21 5 ."

INITSCRIPT_NAME:${PN}-network = "${BPN}-network"
INITSCRIPT_PARAMS:${PN}-network = "start 91 5 ."

do_install() {

	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/${BPN}  ${D}${sbindir}/

	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/${BPN}-early    ${D}${sysconfdir}/init.d/
	install -m 0755 ${WORKDIR}/${BPN}-network  ${D}${sysconfdir}/init.d/
}

FILES:${PN}-early   = "${sysconfdir}/init.d/${BPN}-early"
FILES:${PN}-network = "${sysconfdir}/init.d/${BPN}-network"

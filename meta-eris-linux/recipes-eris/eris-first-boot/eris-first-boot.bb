LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

SRC_URI += "file://900-first-boot"

S = "${WORKDIR}"

do_install() {
	install -d ${D}${sysconfdir}/early-init.d
        install -m 0755 ${WORKDIR}/900-first-boot ${D}${sysconfdir}/early-init.d/900-first-boot
}

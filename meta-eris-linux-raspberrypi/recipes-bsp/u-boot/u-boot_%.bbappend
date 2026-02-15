
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg \
            file://0001-splashscreen-related-environment-variables.patch \
            "

do_install:append() {
	install -d ${D}${sysconfdir}
        echo "/boot/uboot.env 0x0000  0x4000" > ${D}${sysconfdir}/fw_env.config
}

FILES:${PN} += "${sysconfdir}/fw_env.config"


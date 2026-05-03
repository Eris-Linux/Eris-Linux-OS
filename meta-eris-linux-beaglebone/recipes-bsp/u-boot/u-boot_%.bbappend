
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', '', 'file://remove-bootdelay.cfg', d)}"

UBOOT_ENV = ""

ERIS_LOAD_FDT = "1"
ERIS_FDT_IMAGE = "am335x-boneblack.dtb"
ERIS_CONSOLE="console=ttyS0,115200"

do_install:append() {
	install -d ${D}${sysconfdir}
        echo "/boot/uboot.env 0x0000  0x20000" > ${D}${sysconfdir}/fw_env.config
}


FILES:${PN} += "${sysconfdir}/fw_env.config"


FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', '', 'file://remove-bootdelay.cfg', d)}"

SRC_URI += "file://boot-env.cmd"
UBOOT_ENV = "boot-env"
UBOOT_ENV_SUFFIX = "scr"
UBOOT_ENV_BINARY = "boot.scr"
UBOOT_ENV_IMAGE  = "boot-env.scr"
DEPENDS += "u-boot-mkimage-native"


do_install:append() {
	install -d ${D}${sysconfdir}
        echo "/boot/uboot.env 0x0000  0x20000" > ${D}${sysconfdir}/fw_env.config
}

FILES:${PN} += "${sysconfdir}/fw_env.config"


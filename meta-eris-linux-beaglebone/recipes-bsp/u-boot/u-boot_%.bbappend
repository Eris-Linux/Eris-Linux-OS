
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', '', 'file://remove-bootdelay.cfg', d)}"

#SRC_URI += "file://boot-env.cmd"
#UBOOT_ENV = "boot-env"
#UBOOT_ENV = "boot"
#UBOOT_ENV_SUFFIX = "scr"
#UBOOT_ENV_BINARY = "boot.scr"
#UBOOT_ENV_IMAGE  = "boot.scr"
#DEPENDS += "u-boot-mkimage-native"

UBOOT_ENV = ""

ERIS_BLOCK_DEVICE = "0"
ERIS_BOOT_COMMAND = "bootz"
ERIS_GRAPHIC = "0"
ERIS_KERNEL_IMAGE = "zImage"
ERIS_LOAD_FDT = "1"
ERIS_FDT_IMAGE = "am335x-boneblack.dtb"
ERIS_CHOSEN_BOOTARGS = "0"
ERIS_CONSOLE="console=ttyS0,115200"

do_install:append() {
	install -d ${D}${sysconfdir}
        echo "/boot/uboot.env 0x0000  0x20000" > ${D}${sysconfdir}/fw_env.config
}


FILES:${PN} += "${sysconfdir}/fw_env.config"

do_deploy:prepend() {
	install -d ${DEPLOYDIR}
	install -m 0644 boot.scr ${DEPLOYDIR}
}

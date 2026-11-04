
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg"
SRC_URI += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', '', 'file://remove-bootdelay.cfg', d)}"

UBOOT_ENV = ""

ERIS_FDT_IMAGE = "am335x-boneblack.dtb"
ERIS_CONSOLE="console=ttyS0,115200"

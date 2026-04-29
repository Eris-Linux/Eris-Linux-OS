
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg \
            file://0001-splashscreen-related-environment-variables.patch \
            file://boot.cmd.in \
            "

DEPENDS += "u-boot-mkimage-native"

BOOT_MEDIA ?= "mmc"

inherit deploy

# KERNEL_BOOTCMD and KERNEL_IMAGETYPE are filled in meta-rapsberrypi/conf/machine/<machine>.conf

ERIS_KERNEL_IMAGE = "${KERNEL_IMAGETYPE}"
ERIS_GRAPHIC = "0"
ERIS_BLOCK_DEVICE = "0"
ERIS_BOOT_COMMAND = "${KERNEL_BOOT_COMMAND}"

do_compile:prepend() {
	sed -e 's/@@ERIS_KERNEL_IMAGE@@/${ERIS_KERNEL_IMAGE}/' \
	    -e 's/@@ERIS_GRAPHIC@@/${ERIS_GRAPHIC}/' \
	    -e 's/@@ERIS_BLOCK_DEVICE@@/${ERIC_BLOCK_DEVICE}/' \
	    -e 's/@@ERIS_BOOT_COMMAND@@/${ERIS_BOOT_COMMAND}/' \
	    "${WORKDIR}/boot.cmd.in" > "${WORKDIR}/boot.cmd"
	${UBOOT_MKIMAGE} -A ${UBOOT_ARCH} -T script -C none -n "Boot script" -d "${WORKDIR}/boot.cmd" boot.scr
	install -d ${DEPLOYDIR}
	install -m 0644 boot.scr ${DEPLOYDIR}
}



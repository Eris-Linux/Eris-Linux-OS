
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "                      \
    file://boot.cmd.in            \
    file://eris-splashscreen.bmp  \
    "

do_install:append() {
	install -d ${d}/boot
	install -m 0644 ${WORKDIR}/eris-splashscreen.bmp  ${D}/boot/
}

do_deploy:append() {
	install -d ${DEPLOYDIR}
	install -m 0644 ${WORKDIR}/eris-splashscreen.bmp  ${DEPLOYDIR}/
}


DEPENDS += "u-boot-mkimage-native"

inherit deploy

do_compile:prepend() {
	sed                                                        \
	  -e 's/@@ERIS_BLOCK_DEVICE@@/${ERIS_BLOCK_DEVICE}/'       \
	  -e 's/@@ERIS_BOOT_COMMAND@@/${ERIS_BOOT_COMMAND}/'       \
	  -e 's/@@ERIS_GRAPHIC@@/${ERIS_GRAPHIC}/'                 \
	  -e 's/@@ERIS_KERNEL_IMAGE@@/${ERIS_KERNEL_IMAGE}/'       \
	  -e 's/@@ERIS_LOAD_FDT@@/${ERIS_LOAD_FDT}/'               \
	  -e 's/@@ERIS_FDT_IMAGE@@/${ERIS_FDT_IMAGE}/'             \
	  -e 's/@@ERIS_CHOSEN_BOOTARGS@@/${ERIS_CHOSEN_BOOTARGS}/' \
	  -e 's/@@ERIS_CONSOLE@@/${ERIS_CONSOLE}/'                 \
	    "${WORKDIR}/boot.cmd.in" > "${WORKDIR}/boot.cmd"
	${UBOOT_MKIMAGE} -A ${UBOOT_ARCH} -T script -C none -n "Boot script" -d "${WORKDIR}/boot.cmd" boot.scr
	install -d ${DEPLOYDIR}
	install -m 0644 boot.scr ${DEPLOYDIR}
}

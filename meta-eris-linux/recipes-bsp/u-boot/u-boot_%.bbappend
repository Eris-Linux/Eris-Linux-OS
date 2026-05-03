
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "                      \
    file://boot.cmd.in            \
    file://eris-splashscreen.bmp  \
    "

DEPENDS += "u-boot-mkimage-native"

inherit deploy

ERIS_BLOCK_DEVICE    ?= "0"
ERIS_BOOT_COMMAND    ?= "bootz"
ERIS_BOOT_MEDIA      ?= "mmc"
ERIS_CHOSEN_BOOTARGS ?= "0"
ERIS_CONSOLE         ?= ""
ERIS_FDT_ADDR_VAR    ?= "fdt_addr_r"
ERIS_FDT_IMAGE       ?= ""
ERIS_GRAPHIC         ?= "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', 1, 0, d)}"
ERIS_KERNEL_IMAGE    ?= "zImage"
ERIS_LOAD_FDT        ?= "0"

do_compile:prepend() {
	sed                                                        \
	  -e 's/@@ERIS_BLOCK_DEVICE@@/${ERIS_BLOCK_DEVICE}/'       \
	  -e 's/@@ERIS_BOOT_COMMAND@@/${ERIS_BOOT_COMMAND}/'       \
	  -e 's/@@ERIS_BOOT_MEDIA@@/${ERIS_BOOT_MEDIA}/'           \
	  -e 's/@@ERIS_CHOSEN_BOOTARGS@@/${ERIS_CHOSEN_BOOTARGS}/' \
	  -e 's/@@ERIS_CONSOLE@@/${ERIS_CONSOLE}/'                 \
	  -e 's/@@ERIS_FDT_ADDR_VAR@@/${ERIS_FDT_ADDR_VAR}/'       \
	  -e 's/@@ERIS_FDT_IMAGE@@/${ERIS_FDT_IMAGE}/'             \
	  -e 's/@@ERIS_GRAPHIC@@/${ERIS_GRAPHIC}/'                 \
	  -e 's/@@ERIS_KERNEL_IMAGE@@/${ERIS_KERNEL_IMAGE}/'       \
	  -e 's/@@ERIS_LOAD_FDT@@/${ERIS_LOAD_FDT}/'               \
	    "${WORKDIR}/boot.cmd.in" > "${WORKDIR}/boot.cmd"
	${UBOOT_MKIMAGE} -A ${UBOOT_ARCH} -T script -C none -n "Boot script" -d "${WORKDIR}/boot.cmd" "${WORKDIR}/boot.scr"
}


do_install:append() {
	install -d ${d}/boot
	install -m 0644 ${WORKDIR}/eris-splashscreen.bmp  ${D}/boot/
}


do_deploy:prepend() {
	install -d ${DEPLOYDIR}
	install -m 0644 ${WORKDIR}/boot.scr ${DEPLOYDIR}
}

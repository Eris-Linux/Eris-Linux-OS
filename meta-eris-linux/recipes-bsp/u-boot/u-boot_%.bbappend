
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "                      \
    file://boot.cmd.in            \
    file://gpio-fragment.cfg      \
    "

#    file://eris-splashscreen.bmp  \
#

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
ERIS_RESET_GPIO_IN   ?= ""
ERIS_RESET_GPIO_OUT  ?= ""
ERIS_BOOT_PART       ?= "1"
ERIS_SYSTEM_A_PART   ?= "2"
ERIS_SYSTEM_B_PART   ?= "3"
ERIS_DATA_PART       ?= "4"
ERIS_FDT_MODE        ?= "load"
ERIS_DEBUG_DISPLAY   ?= "0"
ERIS_RECOVERY_RAMFS  ?= "recovery-ramfs.cpio.gz"
ERIS_UBOOT_LED_GPIO  ?= ""
ERIS_UBOOT_LED_POWER ?= "1"

do_compile:prepend() {
	sed                                                         \
	  -e 's|@@ERIS_BOOT_PART@@|${ERIS_BOOT_PART}|'              \
	  -e 's|@@ERIS_SYSTEM_A_PART@@|${ERIS_SYSTEM_A_PART}|'      \
	  -e 's|@@ERIS_SYSTEM_B_PART@@|${ERIS_SYSTEM_B_PART}|'      \
	  -e 's|@@ERIS_DATA_PART@@|${ERIS_DATA_PART}|'              \
	  -e 's|@@ERIS_FDT_MODE@@|${ERIS_FDT_MODE}|'                \
	  -e 's|@@ERIS_DEBUG_DISPLAY@@|${ERIS_DEBUG_DISPLAY}|'      \
	  -e 's|@@ERIS_RECOVERY_RAMFS@@|${ERIS_RECOVERY_RAMFS}|'    \
	  -e 's|@@ERIS_UBOOT_LED_GPIO@@|${ERIS_UBOOT_LED_GPIO}|'    \
	  -e 's|@@ERIS_UBOOT_LED_POWER@@|${ERIS_UBOOT_LED_POWER}|'  \
	  -e 's|@@ERIS_BLOCK_DEVICE@@|${ERIS_BLOCK_DEVICE}|'        \
	  -e 's|@@ERIS_BOOT_COMMAND@@|${ERIS_BOOT_COMMAND}|'        \
	  -e 's|@@ERIS_BOOT_MEDIA@@|${ERIS_BOOT_MEDIA}|'            \
	  -e 's|@@ERIS_CHOSEN_BOOTARGS@@|${ERIS_CHOSEN_BOOTARGS}|'  \
	  -e 's|@@ERIS_CONSOLE@@|${ERIS_CONSOLE}|'                  \
	  -e 's|@@ERIS_FDT_ADDR_VAR@@|${ERIS_FDT_ADDR_VAR}|'        \
	  -e 's|@@ERIS_FDT_IMAGE@@|${ERIS_FDT_IMAGE}|'              \
	  -e 's|@@ERIS_GRAPHIC@@|${ERIS_GRAPHIC}|'                  \
	  -e 's|@@ERIS_KERNEL_IMAGE@@|${ERIS_KERNEL_IMAGE}|'        \
	  -e 's|@@ERIS_RESET_GPIO_IN@@|${ERIS_RESET_GPIO_IN}|'      \
	  -e 's|@@ERIS_RESET_GPIO_OUT@@|${ERIS_RESET_GPIO_OUT}|'    \
	    "${WORKDIR}/boot.cmd.in" > "${WORKDIR}/boot.cmd"

	if grep -q '@ERIS_' "${WORKDIR}/boot.cmd"
	then
		bbfatal "Unresolved ERIS placeholder in generated U-Boot script."
	fi
	${UBOOT_MKIMAGE} -A ${UBOOT_ARCH} -T script -C none -n "Boot script" -d "${WORKDIR}/boot.cmd" "${WORKDIR}/boot.scr"

}

 
#do_install:append() {
#	install -d ${D}/boot
#	install -m 0644 ${WORKDIR}/eris-splashscreen.bmp  ${D}/boot/
#}


do_deploy:prepend() {
	install -d ${DEPLOYDIR}
	install -m 0644 ${WORKDIR}/boot.scr ${DEPLOYDIR}
}


#FILES:${PN} += "/boot/eris-splashscreen.bmp"

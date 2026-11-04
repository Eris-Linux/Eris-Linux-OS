
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "                      \
    file://boot.cmd.in            \
    file://gpio-fragment.cfg      \
    "

#    file://eris-splashscreen.bmp  \
#

DEPENDS += "u-boot-mkimage-native"

inherit deploy eris-uboot-env

python eris_prepare_mmc_env() {
    if d.getVar('ERIS_UBOOT_ENV_RAW') == '1':
        import os
        with open(os.path.join(d.getVar('WORKDIR'), 'eris-mmc-env.cfg'), 'w') as fragment:
            fragment.write(eris_uboot_env_fragment(d))
}
do_configure[prefuncs] += "eris_prepare_mmc_env"

# cml1's find_cfgs() reads file:// fragments directly from the layer, rather
# than their unpacked WORKDIR copies. Merge our generated fragment explicitly,
# after the vendor fragments, and resolve Kconfig again.
do_configure:append() {
	if [ "${ERIS_UBOOT_ENV_RAW}" = "1" ]; then
		for configdir in ${@' '.join(d.getVar('B') + '/' + config for config in d.getVar('UBOOT_MACHINE').split()) if d.getVar('UBOOT_CONFIG') else d.getVar('B')}; do
			merge_config.sh -m -O "$configdir" "$configdir/.config" "${WORKDIR}/eris-mmc-env.cfg"
			oe_runmake -C ${S} O="$configdir" olddefconfig
		done
	fi
}

python eris_check_mmc_env() {
    if d.getVar('ERIS_UBOOT_ENV_RAW') != '1':
        return
    import os
    directories = [d.getVar('B')]
    if d.getVar('UBOOT_CONFIG'):
        directories = [os.path.join(d.getVar('B'), config) for config in d.getVar('UBOOT_MACHINE').split()]
    expected = eris_uboot_env_fragment(d).splitlines()
    for directory in directories:
        with open(os.path.join(directory, '.config')) as config:
            actual = set(config.read().splitlines())
        for line in expected:
            # Options absent from this U-Boot version need not have a # line.
            if line.startswith('# '):
                option = line.split()[1]
                if any(value.startswith(option + '=') for value in actual):
                    bb.fatal('%s is unexpectedly enabled in %s' % (option, directory))
            elif line not in actual:
                bb.fatal('Missing required MMC environment setting %s in %s' % (line, directory))
}
do_configure[postfuncs] += "eris_check_mmc_env"

# The same package supplies Linux and recovery. Remove the vendor FAT config
# after all board-specific installation hooks have run.
eris_cleanup_vendor_env_config() {
	rm -f ${D}${sysconfdir}/fw_env.config
}
do_install[postfuncs] += "eris_cleanup_vendor_env_config"
RDEPENDS:${PN}-env += "eris-uboot-env"

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
ERIS_RECOVERY_RAMFS  ?= "${@'recovery-ramfs.cpio.gz.u-boot' if d.getVar('ERIS_BOOT_COMMAND') == 'bootm' else 'recovery-ramfs.cpio.gz'}"
ERIS_UBOOT_LED_GPIO  ?= ""
ERIS_UBOOT_LED_POWER ?= "1"

ERIS_BOOT_STATUS_STABLE = "stable"
ERIS_BOOT_STATUS_TRIAL = "trial"
ERIS_BOOT_STATUS_ROLLBACK = "rollback"
ERIS_BOOT_STATUS_REPAIR = "repair"
ERIS_BOOT_STATUS_RESET = "reset"
ERIS_BOOT_STATUS_RESCUE = "rescue"
ERIS_MAX_BOOT_ATTEMPTS = "3"

do_compile:prepend() {
	sed                                                         \
	  -e 's|@@ERIS_BOOT_STATUS_STABLE@@|${ERIS_BOOT_STATUS_STABLE}|g' \
	  -e 's|@@ERIS_BOOT_STATUS_TRIAL@@|${ERIS_BOOT_STATUS_TRIAL}|g' \
	  -e 's|@@ERIS_BOOT_STATUS_ROLLBACK@@|${ERIS_BOOT_STATUS_ROLLBACK}|g' \
	  -e 's|@@ERIS_BOOT_STATUS_REPAIR@@|${ERIS_BOOT_STATUS_REPAIR}|g' \
	  -e 's|@@ERIS_BOOT_STATUS_RESET@@|${ERIS_BOOT_STATUS_RESET}|g' \
	  -e 's|@@ERIS_BOOT_STATUS_RESCUE@@|${ERIS_BOOT_STATUS_RESCUE}|g' \
	  -e 's|@@ERIS_MAX_BOOT_ATTEMPTS@@|${ERIS_MAX_BOOT_ATTEMPTS}|g' \
	  -e 's|@@ERIS_BOOT_PART@@|${ERIS_BOOT_PART}|g'              \
	  -e 's|@@ERIS_SYSTEM_A_PART@@|${ERIS_SYSTEM_A_PART}|g'      \
	  -e 's|@@ERIS_SYSTEM_B_PART@@|${ERIS_SYSTEM_B_PART}|g'      \
	  -e 's|@@ERIS_DATA_PART@@|${ERIS_DATA_PART}|g'              \
	  -e 's|@@ERIS_FDT_MODE@@|${ERIS_FDT_MODE}|g'                \
	  -e 's|@@ERIS_DEBUG_DISPLAY@@|${ERIS_DEBUG_DISPLAY}|g'      \
	  -e 's|@@ERIS_RECOVERY_RAMFS@@|${ERIS_RECOVERY_RAMFS}|g'    \
	  -e 's|@@ERIS_UBOOT_LED_GPIO@@|${ERIS_UBOOT_LED_GPIO}|g'    \
	  -e 's|@@ERIS_UBOOT_LED_POWER@@|${ERIS_UBOOT_LED_POWER}|g'  \
	  -e 's|@@ERIS_BLOCK_DEVICE@@|${ERIS_BLOCK_DEVICE}|g'        \
	  -e 's|@@ERIS_BOOT_COMMAND@@|${ERIS_BOOT_COMMAND}|g'        \
	  -e 's|@@ERIS_BOOT_MEDIA@@|${ERIS_BOOT_MEDIA}|g'            \
	  -e 's|@@ERIS_CHOSEN_BOOTARGS@@|${ERIS_CHOSEN_BOOTARGS}|g'  \
	  -e 's|@@ERIS_CONSOLE@@|${ERIS_CONSOLE}|g'                  \
	  -e 's|@@ERIS_FDT_ADDR_VAR@@|${ERIS_FDT_ADDR_VAR}|g'        \
	  -e 's|@@ERIS_FDT_IMAGE@@|${ERIS_FDT_IMAGE}|g'              \
	  -e 's|@@ERIS_GRAPHIC@@|${ERIS_GRAPHIC}|g'                  \
	  -e 's|@@ERIS_KERNEL_IMAGE@@|${ERIS_KERNEL_IMAGE}|g'        \
	  -e 's|@@ERIS_RESET_GPIO_IN@@|${ERIS_RESET_GPIO_IN}|g'      \
	  -e 's|@@ERIS_RESET_GPIO_OUT@@|${ERIS_RESET_GPIO_OUT}|g'    \
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

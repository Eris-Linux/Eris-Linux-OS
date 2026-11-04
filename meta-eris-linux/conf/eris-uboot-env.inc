# One layout shared by U-Boot, Linux, recovery and the installation image.
# Offsets are bytes from the start of the MMC user area, not from BOOT.
ERIS_UBOOT_ENV_RAW ?= "0"
ERIS_UBOOT_ENV_RAW:rpi = "1"
ERIS_UBOOT_ENV_RAW:beaglebone-yocto = "1"
ERIS_UBOOT_ENV_MMC_DEVICE ?= "0"
ERIS_UBOOT_ENV_DEVICE ?= "/dev/mmcblk0"
ERIS_UBOOT_ENV_OFFSET ?= "0x100000"
ERIS_UBOOT_ENV_OFFSET_REDUND ?= "0x200000"
ERIS_UBOOT_ENV_SIZE ?= "0x4000"
ERIS_UBOOT_ENV_SIZE:beaglebone-yocto = "0x20000"
ERIS_UBOOT_BOOT_OFFSET ?= "0x400000"

ERIS_UBOOT_ENV_OFFSET_KIB = "${@int(d.getVar('ERIS_UBOOT_ENV_OFFSET'), 0) // 1024}"
ERIS_UBOOT_ENV_OFFSET_REDUND_KIB = "${@int(d.getVar('ERIS_UBOOT_ENV_OFFSET_REDUND'), 0) // 1024}"
ERIS_UBOOT_ENV_SIZE_KIB = "${@int(d.getVar('ERIS_UBOOT_ENV_SIZE'), 0) // 1024}"
ERIS_UBOOT_BOOT_OFFSET_KIB = "${@int(d.getVar('ERIS_UBOOT_BOOT_OFFSET'), 0) // 1024}"
ERIS_UBOOT_ENV_IMAGE = "eris-uboot-env-blank-${MACHINE}.bin"

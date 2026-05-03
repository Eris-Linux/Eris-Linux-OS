
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg \
            file://0001-splashscreen-related-environment-variables.patch \
            "

BOOT_MEDIA ?= "mmc"

# KERNEL_BOOTCMD and KERNEL_IMAGETYPE are filled in meta-raspberrypi/conf/machine/<machine>.conf
ERIS_BLOCK_DEVICE = "0"
ERIS_BOOT_COMMAND = "${KERNEL_BOOT_COMMAND}"
ERIS_GRAPHIC = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', 1, 0, d)}"
ERIS_KERNEL_IMAGE = "${KERNEL_IMAGETYPE}"
ERIS_LOAD_FDT = "1"
ERIS_FDT_IMAGE = ""
ERIS_CHOSEN_BOOTARGS = "1"
ERIS_CONSOLE = ""

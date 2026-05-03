
FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += "file://eris-linux-fragment.cfg \
            file://0001-splashscreen-related-environment-variables.patch \
            "

# KERNEL_BOOTCMD and KERNEL_IMAGETYPE are filled in meta-raspberrypi/conf/machine/<machine>.conf
ERIS_BOOT_COMMAND = "${KERNEL_BOOTCMD}"
ERIS_KERNEL_IMAGE = "${KERNEL_IMAGETYPE}"
ERIS_FDT_ADDR_VAR = "fdt_addr"
ERIS_CHOSEN_BOOTARGS = "1"

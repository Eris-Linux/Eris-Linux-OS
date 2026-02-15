# U-boot script for eris-linux update system
#
# (c) 2024 Logilin
#
#
# We use the following environment variables
# - current_boot
# - next_boot_1
# - next_boot_2
# - next_boot_3
# - recovery_part
# - rollback: 0 normal boot, 1 update, 2 rollback


#cls
load mmc 0:2 ${loadaddr} /boot/eris-splashscreen-01.bmp
bmp display ${loadaddr}

# Set a default configuration
if test "${boot_next_1}" = ""
then
	# Default to recovery
	setenv  recovery_part 2
	setenv  current_boot  ${recovery_part}
	setenv  boot_next_1   ${recovery_part}
	setenv  boot_next_2   ${recovery_part}
	setenv  boot_next_3   ${recovery_part}
	setenv  rollback      0
	saveenv
else
	if test "${current_boot}" != "${boot_next_1}"
	then
		if test "${rollback}" = "0"
		then
			# Previous boot as failed.
			setenv roolback 2
		fi
		if test "${rollback}" = "1"
		then
			# First boot after update.
			setenv rollback 0
		fi
	fi
	setenv current_boot ${boot_next_1}
	setenv boot_next_1  ${boot_next_2}
	setenv boot_next_2  ${boot_next_3}
	setenv boot_next_3  ${recovery_part}
	saveenv
fi

# bootdev = 0 -> microSD.
# bootdev = 1 -> eMMC
setenv bootdev 0

load mmc ${bootdev}:${current_boot} ${kernel_addr_r} boot/zImage

setenv fdt_addr 0x88000000
load mmc ${bootdev}:${current_boot} ${fdt_addr}  /boot/am335x-boneblack.dtb
fdt addr ${fdt_addr}

setenv bootargs "root=/dev/mmcblk${bootdev}p${current_boot} console=ttyS0,115200 rootwait ro init=/sbin/early-init"
bootz ${kernel_addr_r} - ${fdt_addr}

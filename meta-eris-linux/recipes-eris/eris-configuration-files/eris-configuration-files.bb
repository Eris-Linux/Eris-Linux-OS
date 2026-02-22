SUMMARY = "Eris-Linux default configuration files."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI += "file://system-public-key.pem"

do_install() {

	# /usr/share/eris-linux/system-version

	install -d ${D}${datadir}/eris-linux/
	install -d ${D}${datadir}/eris-linux/containers
	echo "${MACHINE}"              > ${D}${datadir}/eris-linux/machine
	echo "${ERIS_SYSTEM_TYPE}"     > ${D}${datadir}/eris-linux/system-type
	echo "${ERIS_SYSTEM_MODEL}"    > ${D}${datadir}/eris-linux/system-model
	echo "${ERIS_SYSTEM_VERSION}"  > ${D}${datadir}/eris-linux/system-version

	install -m 0400 ${WORKDIR}/system-public-key.pem ${D}${datadir}/eris-linux/

	# /etc/eris-linux/

	install -d ${D}${sysconfdir}/eris-linux

	# /etc/eris-linux/parameters

	echo "# Eris Linux parameters file."                                       >  ${D}${sysconfdir}/eris-linux/parameters
	echo "# "                                                                  >> ${D}${sysconfdir}/eris-linux/parameters
	echo "# Feel free to modify parameters below to configure Eris behavior."  >> ${D}${sysconfdir}/eris-linux/parameters
	echo "# But you could better use the Eris API or device manager."          >> ${D}${sysconfdir}/eris-linux/parameters
	echo ""                                                                    >> ${D}${sysconfdir}/eris-linux/parameters
	echo "status_upload_period_seconds=300"                                    >> ${D}${sysconfdir}/eris-linux/parameters
	echo "automatic_system_update=y"                                           >> ${D}${sysconfdir}/eris-linux/parameters
	echo "automatic_reboot_after_update=n"                                     >> ${D}${sysconfdir}/eris-linux/parameters
	echo "ntp_server=pool.ntp.org"                                             >> ${D}${sysconfdir}/eris-linux/parameters
	echo "ntp_enable=yes"                                                      >> ${D}${sysconfdir}/eris-linux/parameters
	echo "container_update_policy=immediate"                                   >> ${D}${sysconfdir}/eris-linux/parameters

	# /etc/eris-linux/partitions

	echo "ERIS_STORAGE_DEVICE=/dev/mmcblk0"  > ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_SEPARATOR=p"       >> ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_BOOT=1"            >> ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_SYSTEM_R=2"        >> ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_SYSTEM_A=3"        >> ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_EXTENDED=4"        >> ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_SYSTEM_B=5"        >> ${D}${datadir}/eris-linux/partitions
	echo "ERIS_PARTITION_DATA=6"            >> ${D}${datadir}/eris-linux/partitions
}

FILES:${PN} += "${datadir}/eris-linux/containers"
FILES:${PN} += "${datadir}/eris-linux/machine"
FILES:${PN} += "${datadir}/eris-linux/partitions"
FILES:${PN} += "${datadir}/eris-linux/system-type"
FILES:${PN} += "${datadir}/eris-linux/system-model"
FILES:${PN} += "${datadir}/eris-linux/system-public-key.pem"
FILES:${PN} += "${datadir}/eris-linux/system-version"
FILES:${PN} += "${sysconfdir}/eris-linux/parameters"

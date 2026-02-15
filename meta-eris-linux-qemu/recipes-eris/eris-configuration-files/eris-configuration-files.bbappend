
do_install:append() {
	echo "ERIS_PARTITION_DATA=/dev/vdb"       >> ${D}${datadir}/eris-linux/partitions
}

FILES:${PN} += "${datadir}/eris-linux/partitions"

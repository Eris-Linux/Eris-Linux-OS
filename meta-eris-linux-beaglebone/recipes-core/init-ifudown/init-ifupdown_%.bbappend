do_install:append() {

	echo "auto eth0"                                                   >  ${D}${sysconfdir}/network/interfaces
	echo "iface eth0 inet dhcp"                                        >> ${D}${sysconfdir}/network/interfaces
}

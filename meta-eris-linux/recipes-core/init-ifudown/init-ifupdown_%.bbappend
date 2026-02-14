#
# `init-ifupdown_%.bbappend`
#
# (c) 2024-2026 Logilin.
#

do_install:append() {

	# Install a default `/etc/network/interfaces` file.

	install -d ${D}${sysconfdir}/network

	echo "auto lo"                        >  ${D}${sysconfdir}/network/interfaces
	echo "iface lo inet loopback"         >> ${D}${sysconfdir}/network/interfaces
	echo ""                               >> ${D}${sysconfdir}/network/interfaces
}

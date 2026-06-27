
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:raspberrypi5 = " file://xorg-conf-d-99-vc4-conf-raspberrypi5"

do_install:append:raspberrypi5() {
	install -d ${D}${sysconfdir}/X11/xorg.conf.d
	install -m 0755 ${WORKDIR}/xorg-conf-d-99-vc4-conf-raspberrypi5  ${D}${sysconfdir}/X11/xorg.conf.d/99-vc4.conf
}


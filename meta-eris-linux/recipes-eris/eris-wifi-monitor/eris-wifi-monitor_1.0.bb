SUMMARY = "Eris Wifi monitor"
AUTHOR = "Christophe BLAESS"
DESCRIPTION = "Handle network connection using Wifi interface."

LICENSE = "LGPL-2.1-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/${LICENSE};md5=1a6d268fd218675ffea8be556788b780"

SRC_URI += "                                        \
    file://run-eris-wifi-monitor                    \
    file://eris-wifi-monitor                        \
           "

inherit update-rc.d
INITSCRIPT_NAME = "run-eris-wifi-monitor"
INITSCRIPT_PARAMS = "start 90 2 3 4 5 ."

do_install:append() {
        install -d ${D}${sysconfdir}
        install -d ${D}${sysconfdir}/init.d
        install -m 755 ${WORKDIR}/run-eris-wifi-monitor ${D}${sysconfdir}/init.d/
	install -d ${D}${sbindir}
	install -m 755 ${WORKDIR}/eris-wifi-monitor ${D}${sbindir}/
}


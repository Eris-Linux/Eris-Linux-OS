
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://no-screen-saver"

do_install:append() {
	install -d ${D}${bindir}
	install -m 0755 ${WORKDIR}/no-screen-saver  ${D}${bindir}
}

INITSCRIPT_PARAMS = "start 0 5 . stop 20 0 1 2 3 6 ."


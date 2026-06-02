SUMMARY = "Ersi API library"
DESCRIPTION = "This shared library is used inside the containers to access Eris API"
LICENSE = "CLOSED"

SRC_URI = "git://github.com/Eris-Linux/liberis.git;protocol=https;branch=master"
PV = "1.0+git${SRCPV}"
SRCREV = "eaf8f94db1504611a249f1ff8ed13dbc6aed5842"

DEPENDS += "curl"

S = "${WORKDIR}"

do_compile() {
	oe_runmake
}

do_install() {
	install -d ${D}${libdir}
	install -m 0755 ${PN}.so.${PV} ${D}${libdir}/
	ln -sf ${PN}.so.${PV} ${D}${libdir}/${PN}.so

	install -d ${D}${includedir}
	install -m 0644 ${PN}.h ${D}${includedir}/
}

FILES:${PN} = "${libdir}/${PN}.so.*"
FILES:${PN}-dev = "${includedir} \
                   ${libdir}/${PN}.so*"

INSANE_SKIP:${PN} = "dev-so"


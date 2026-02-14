SUMMARY = "Ersi API library"
DESCRIPTION = "This shared library is used inside the containers to access Eris API"
LICENSE = "CLOSED"

SRC_URI = "file://${BPN}.c \
           file://${BPN}.h \
           file://Makefile"

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

# Définir les paquets
# - ${PN} : binaire/runtime (libexample.so.1.0, etc.)
# - ${PN}-dev : fichiers de dev (headers, symlink libexample.so)
FILES:${PN} = "${libdir}/${PN}.so.*"
FILES:${PN}-dev = "${includedir} \
                   ${libdir}/${PN}.so*"

# Pour que le SDK contienne les dev files
#RDEPENDS:${PN}-dev += "${PN}"

# On sépare bien le runtime et le dev
INSANE_SKIP:${PN} = "dev-so"

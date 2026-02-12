SUMMARY = "Graphical background for the Eris system when no container are running."
LICENSE = "CLOSED"

SRC_URI += "file://eris-dashboard.c"
SRC_URI += "file://Makefile"

DEPENDS = "libx11"

S = "${WORKDIR}"

do_compile () {
	sed -i -e 's/^#define ERIS_SYSTEM_VERSION.*$/#define ERIS_SYSTEM_VERSION "'${ERIS_SYSTEM_VERSION}'"/g' eris-dashboard.c
	oe_runmake
}

do_compile[nostamp] = "1"

do_install () {
	install -d ${D}${bindir}
	install -m 0755 ${S}/${PN} ${D}${bindir}
}


DESCRIPTION = "Eris-Linux REST API between containers and host."
LICENSE = "CLOSED"

SRC_URI="                    \
  file://addsnprintf.c       \
  file://addsnprintf.h       \
  file://eris-rest-api.c     \
  file://eris-rest-api.h     \
  file://gpio-rest-api.c     \
  file://gpio-rest-api.h     \
  file://net-rest-api.c      \
  file://net-rest-api.h      \
  file://sbom-rest-api.c     \
  file://sbom-rest-api.h     \
  file://system-rest-api.c   \
  file://system-rest-api.h   \
  file://time-rest-api.c     \
  file://time-rest-api.h     \
  file://update-rest-api.c   \
  file://update-rest-api.h   \
  file://wdog-rest-api.c     \
  file://wdog-rest-api.h     \
  file://Makefile            \
  file://start-${BPN}        \
  file://${BPN}.service      \
"

DEPENDS += "libgpiod"
DEPENDS += "libmicrohttpd"
DEPENDS += "util-linux-libuuid"

S = "${WORKDIR}"

inherit update-rc.d
INITSCRIPT_NAME = "start-${BPN}"
INITSCRIPT_PARAMS = "start 19 5 ."

inherit systemd
SYSTEMD_SERVICE:${PN} += "${BPN}.service"

do_compile() {
	oe_runmake
}

do_install() {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/start-${BPN} ${D}${sysconfdir}/init.d/

	install -d ${D}/${systemd_system_unitdir}
	install -m 0644 ${WORKDIR}/${BPN}.service ${D}/${systemd_system_unitdir}/

	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/${BPN}  ${D}${sbindir}
}

FILES:${PN} += "${systemd_system_unitdir}/${BPN}.service"

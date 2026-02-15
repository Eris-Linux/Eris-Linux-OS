DESCRIPTION = "Eris-Linux API server between containers and host."
LICENSE = "CLOSED"

SRC_URI="                    \
  file://api-server.c        \
  file://api-server.h        \
  file://gpio-api.c          \
  file://gpio-api.h          \
  file://leds-api.c          \
  file://leds-api.h          \
  file://net-api.c           \
  file://net-api.h           \
  file://procfs-api.c        \
  file://procfs-api.h        \
  file://sbom-api.c          \
  file://sbom-api.h          \
  file://system-api.c        \
  file://system-api.h        \
  file://time-api.c          \
  file://time-api.h          \
  file://update-api.c        \
  file://update-api.h        \
  file://wdog-api.c          \
  file://wdog-api.h          \
  file://Makefile            \
  file://start-${BPN}        \
  file://${BPN}.service      \
"

DEPENDS += "libgpiod"
DEPENDS += "util-linux-libuuid"

S = "${WORKDIR}"

inherit update-rc.d
INITSCRIPT_NAME = "start-${BPN}"
INITSCRIPT_PARAMS = "start 19 5 ."

inherit systemd
SYSTEMD_SERVICE:${PN} += "${BPN}.service"

TEST_INPUT_GPIO  ??= ""
TEST_OUTPUT_GPIO ??= ""

do_compile() {
	sed -i -e 's/@TEST_INPUT_GPIO@/${TEST_INPUT_GPIO}/g' ${S}/gpio-api.c
	sed -i -e 's/@TEST_OUTPUT_GPIO@/${TEST_OUTPUT_GPIO}/g' ${S}/gpio-api.c

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

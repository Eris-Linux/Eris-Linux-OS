SUMMARY = "Minimal X client to keep server running even if the main application terminates."
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

SRC_URI = "git://github.com/cpb-/dummy-x-client;protocol=https;branch=master"

PV = "1.0+git"
SRCREV = "6924a5c720212b7295061a30f961286cb8d534f4"

DEPENDS = "libx11"

S = "${WORKDIR}/git"

do_compile () {
	oe_runmake
}

do_install () {
	install -d ${D}${bindir}
	install -m 0755 ${S}/${PN} ${D}${bindir}
}


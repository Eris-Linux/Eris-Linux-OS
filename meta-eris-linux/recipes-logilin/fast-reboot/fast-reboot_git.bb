LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "git://github.com/cpb-/fast-reboot.git;protocol=https;branch=main"

PV = "1.0+git${SRCPV}"
SRCREV = "65fcb752d86bafddaf93d4a8303e085ae537a72e"

S = "${WORKDIR}/git"

do_compile () {
        oe_runmake
}

do_install () {
        install -d ${D}/${sbindir}
        oe_runmake install 'DESTDIR=${D}/${sbindir}'
}

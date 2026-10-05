FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://daemon.json"

PACKAGECONFIG:remove = "transient-config"

do_install:append() {
    install -d ${D}${sysconfdir}/docker
    install -m 0644 ${WORKDIR}/daemon.json ${D}${sysconfdir}/docker/daemon.json
}

CONFFILES:${PN} += "${sysconfdir}/docker/daemon.json"

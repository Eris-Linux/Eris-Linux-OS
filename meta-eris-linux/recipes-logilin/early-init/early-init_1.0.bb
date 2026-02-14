SUMMARY = "Minimal init system to run tasks before the real init process start on embedded systems."
HOMEPAGE = "https://github.com/cpb-/early-init"
DESCRIPTION = "The aim of this small project is to easily run custom tasks during the boot \
of an embedded system before starting systemd, sysvinit, openrc, or any other init daemon. \
The main idea is to divide the initialization tasks in two parts: the low-level tasks that \
need to be run at the very start of the boot, in a certain order, with a predictible behavior, \
and the high-level tasks that are needed to run custom code on the embedded system."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"


SRC_URI = "git://github.com/cpb-/early-init.git;protocol=https;branch=master"
PV = "1.0+git${SRCPV}"
SRCREV = "5cd1e1df4f5b5614ae69895503840d29a60a0067"

S = "${WORKDIR}/git"

SRC_URI += "file://010-partitionning-at-first-boot"
SRC_URI += "file://020-mount-tmpfs-home"
SRC_URI += "file://030-mount-data-partition"
SRC_URI += "file://040-bind-mount-containers-directory"
SRC_URI += "file://050-mount-overlayfs-on-etc"

do_install () {
	install -d ${D}/${base_sbindir}
	install -m 0755 ${S}/early-init ${D}/${base_sbindir}/
	install -d ${D}/${sysconfdir}/early-init.d

	install -m 0755 ${WORKDIR}/010-partitionning-at-first-boot      ${D}${sysconfdir}/early-init.d 
	install -m 0755 ${WORKDIR}/020-mount-tmpfs-home                 ${D}${sysconfdir}/early-init.d 
	install -m 0755 ${WORKDIR}/030-mount-data-partition             ${D}${sysconfdir}/early-init.d
	install -m 0755 ${WORKDIR}/040-bind-mount-containers-directory  ${D}${sysconfdir}/early-init.d
	install -m 0755 ${WORKDIR}/050-mount-overlayfs-on-etc           ${D}${sysconfdir}/early-init.d
}

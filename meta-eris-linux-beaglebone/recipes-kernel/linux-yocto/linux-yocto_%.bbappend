
SRC_URI += "file://fragment.cfg"
SRC_URI += "file://device-mapper-fragment.cfg"
SRC_URI += "file://f2fs-fragment.cfg"

FILESEXTRAPATHS:append := ":${THISDIR}/files"


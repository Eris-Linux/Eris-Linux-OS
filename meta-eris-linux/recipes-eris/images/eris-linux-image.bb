#
# `eris-linux-image` recipe.
#
# Logilin 2024-2025
#

IMAGE_BASENAME = "eris-linux"
IMAGE_NAME_SUFFIX = ""
IMAGE_VERSION_SUFFIX="-${ERIS_SYSTEM_VERSION}"
IMAGE_LINK_NAME="${IMAGE_BASENAME}-${ERIS_SYSTEM_MODEL}"


IMAGE_NAME="${IMAGE_BASENAME}_${ERIS_SYSTEM_MODEL}_${ERIS_SYSTEM_VERSION}_${ERIS_SYSTEM_TYPE}"

inherit core-image

# ---------------------------------------------------------------------------
# User access.
#
# During development phase, `root` is allowed to connect without password,
# even with ssh.
#
# Some debug tools are present.
#
# When developement phase is over, no user connection is allowed.
#
inherit extrausers

IMAGE_FEATURES += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', 'allow-empty-password allow-root-login empty-root-password', '', d)}"

EXTRA_USERS_PARAMS += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', '', 'usermod -L -e 1 root;', d)}"
EXTRA_USERS_PARAMS += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', '', 'usermod -s /sbin/nologin root;', d)}"

IMAGE_FEATURES += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', 'ssh-server-openssh', '', d)}"

IMAGE_FEATURES += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', 'tools-debug', '', d)}"

# Standard image features
#
IMAGE_FEATURES += "read-only-rootfs"

# Boot and system update related stuff
#
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'no-update',  '', ' u-boot  u-boot-fw-utils kernel-devicetree', d)}"
IMAGE_INSTALL:append = " kernel-image kernel-modules"
IMAGE_INSTALL:append = " e2fsprogs"
IMAGE_INSTALL:append = " fast-reboot"

# Standard command line tools
#
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', ' nano', '', d)}"

# Development and setup tools
#
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', ' dtc', '', d)}"

# Graphical environment
#
IMAGE_FEATURES += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', ' x11', '', d)}"
IMAGE_FEATURES += "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', ' x11-base', '', d)}"
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', ' xorg-fonts-100dpi', '', d)}"
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', ' eris-dashboard', '', d)}"
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', ' xsetroot', '', d)}"
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-graphic', ' qtbase', '', d)}"

# Containers and virtualization
#
IMAGE_INSTALL:append = " docker"
IMAGE_INSTALL:append = " eris-containers"

# Librairies needed by the API
#
IMAGE_INSTALL:append = " libgpiod"
IMAGE_INSTALL:append = " libmicrohttpd"
IMAGE_INSTALL:append = " util-linux-libuuid"

IMAGE_INSTALL:append = " tzdata"
#DEFAULT_TIMEZONE = "UTC"

# Eris packages
#
#IMAGE_INSTALL:append = " eris-api-server"
IMAGE_INSTALL:append = " eris-rest-api"
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', ' eris-api-server-dbg', '', d)}"

IMAGE_INSTALL:append = " eris-ntp"
IMAGE_INSTALL:append = " early-init"
IMAGE_INSTALL:append = " eris-first-boot"
IMAGE_INSTALL:append = " eris-configuration-files"
IMAGE_INSTALL:append = " usb-key-automount"

IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', ' rw-ro', '', d)}"
IMAGE_INSTALL:append = "${@bb.utils.contains('DISTRO_FEATURES', 'eris-devel', ' build-timestamp', '', d)}"

TOOLCHAIN_TARGET_TASK:append = " liberis-dev"

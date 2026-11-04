SUMMARY = "Shared Eris U-Boot environment configuration and MMC image reservations"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit deploy eris-uboot-env

PACKAGE_ARCH = "${MACHINE_ARCH}"
S = "${WORKDIR}"

python do_install() {
    import os
    directory = d.getVar('D') + d.getVar('sysconfdir')
    os.makedirs(directory, exist_ok=True)
    with open(os.path.join(directory, 'fw_env.config'), 'w') as config:
        config.write(eris_uboot_env_config(d))
}

python do_deploy() {
    if d.getVar('ERIS_UBOOT_ENV_RAW') != '1':
        return
    import os
    layout = eris_uboot_env_layout(d)
    os.makedirs(d.getVar('DEPLOYDIR'), exist_ok=True)
    # Deliberately invalid CRC: the first boot uses compiled-in defaults.
    # WIC writes both slots, also when reusing a previously programmed card.
    with open(os.path.join(d.getVar('DEPLOYDIR'), d.getVar('ERIS_UBOOT_ENV_IMAGE')), 'wb') as image:
        image.write(b'\xff' * layout['SIZE'])
}
addtask deploy after do_compile before do_build

FILES:${PN} = "${sysconfdir}/fw_env.config"

#
# `openssh_%.bbappend`
#
# (c) 2024-2026 Logilin.
#

do_install:append() {

	# Store keys on `/data` partition instead on volatile `/var` even in read-only roots mode.

        sed -i '/HostKey/d' ${D}${sysconfdir}/ssh/sshd_config_readonly
        echo "HostKey /etc/ssh/ssh_host_rsa_key"     >> ${D}${sysconfdir}/ssh/sshd_config_readonly
        echo "HostKey /etc/ssh/ssh_host_ecdsa_key"   >> ${D}${sysconfdir}/ssh/sshd_config_readonly
        echo "HostKey /etc/ssh/ssh_host_ed25519_key" >> ${D}${sysconfdir}/ssh/sshd_config_readonly
}

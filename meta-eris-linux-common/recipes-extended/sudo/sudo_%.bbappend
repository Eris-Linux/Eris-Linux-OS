#
# `sudo_%.bbappend`
#
# Logilin 2024.
#

do_install:append() {

	# Uncomment the `sudo` category line in `/etc/sudoers`.

	sed -i 's/^#\s*\(%sudo\s*ALL=(ALL:ALL)\s*ALL\)/\1/'  ${D}${sysconfdir}/sudoers

	# Remove the small warning text at first sudo call:
	#
	#   We trust you have received the usual lecture from the local System
	#   Administrator. It usually boils down to these three things:
	#    #1) Respect the privacy of others.
	#    #2) Think before you type.
	#    #3) With great power comes great responsibility.

	echo "Defaults lecture = never" >> ${D}${sysconfdir}/sudoers
}

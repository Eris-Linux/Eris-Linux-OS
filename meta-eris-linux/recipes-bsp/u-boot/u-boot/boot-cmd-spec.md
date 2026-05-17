# Specifications for boot-cmd script.

If the number of remaining attempt for the candidate partition is
greater than zero, the script must decrease the number of remaining
attempts and boot on candidate partition.

When a boot is successful, a Linux script will reset the number of
remaining attempts to 3.


If the number of attempt is zero and the rollback level is 0 (no rollback),
the script must set the rollback level to 1, switch the candidate partition,
reset the number of attempts to 3 and proceed as in the first case (decrease
and boot on the candidate partition).


If the number of attempt is zero, and the rollback level is 1 (simple rollback),
the script must erase all user data, system data and installed containers,
then set rollback level to 2, reset number of attempts to 3 and proceed as
in the first case.

If the number of attempts is zero and the rollback level is more than 1,
the boot script must boot on the recovery initramfs.


During contact with the device manager, the rollback level is transmitted.
The device manager won't propose anymore an image that failed to boot.


After a successful image install, the rollback level is restored to zero.


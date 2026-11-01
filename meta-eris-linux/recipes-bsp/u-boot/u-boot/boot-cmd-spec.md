# Eris boot protocol

The persistent environment uses these literal state names: `stable`, `trial`,
`rollback`, `repair`, `reset`, `rescue`. SYSTEM-A is partition 2, SYSTEM-B is
partition 3. A stage has three attempts; each attempt is saved before boot.
Failure to save the environment halts boot rather than losing the retry budget.

| State | Action | When the attempt budget is exhausted |
| --- | --- | --- |
| stable | Boot confirmed_partition | repair |
| trial | Boot candidate_partition | rollback |
| rollback | Recovery restores the saved data, then boot confirmed_partition | repair |
| repair | Recovery repairs DATA, then boot confirmed_partition | reset |
| reset | Recovery recreates DATA, then boot confirmed_partition | rescue |
| rescue | Halt for manual intervention | Remain halted |

The reset GPIO requests `reset`, clears `recovery_done`, and resets the budget.
Kernel, DTB, UUID or ramdisk loading failures trigger a reboot. Recovery boots
also consume attempts: crashes and power failures eventually reach rescue.
An uninitialized environment starts on SYSTEM-A in stable state. When migrating
a legacy environment, its valid candidate partition is preserved as confirmed. An invalid
confirmed partition enters rescue; an invalid candidate exhausts trial.

## Linux tools

`force-boot-on-partition` and the updater schedule a trial, preserving the
confirmed partition. `boot-ok` confirms the actual running partition only after
the optional application health check passes, clears the candidate, returns to
stable and resets the budget. It must never infer the running root partition
from a pending candidate in the environment.

`rollback_level` remains a manager reporting field: 1 for rollback and 2 for
reset. `failed_partition` records an exhausted trial for diagnostics.
A new image installation clears the reporting level; boot confirmation preserves it.

## Local rejection of failed image versions

After authenticating the archive signature, the updater reads
`usr/share/eris-linux/system-version` from the rootfs tarball. It rejects a
version equal to persistent `failed_version` before writing the inactive
partition. Invalid metadata or an unreadable boot environment also prevent
installation. The image URL is not used as the version identifier.

After installation, the updater saves `candidate_version`, candidate partition,
`boot_state=trial`, `boot_attempts=3`, `recovery_done=0` and `rollback_level=0`
in one environment store, checking the failed version again before scheduling.
When trial attempts are exhausted, U-Boot copies nonempty `candidate_version`
to `failed_version` in the same store as the transition to rollback.

Boot confirmation removes `candidate_version` but preserves `failed_version`.
Scheduling a new version does not clear it. Manual partition selection clears
the candidate version, so an unversioned manual trial cannot erase a recorded
failure. Only the last failed version is remembered. A corrected image must
have a new version; this is a local policy, with no new Device Manager field.

Rejected known-failed versions do not consume the updater's installation error
budget or schedule a reboot. The server may still propose the image, so it can
still be downloaded and authenticated before local rejection.

Deploy the updated boot.scr AND updater, including in the confirmed fallback
rootfs. An older updater on the fallback system cannot enforce this policy.
For a fresh board test, install a normal SD image containing these changes,
then send a higher-version image with deliberately failing boot validation.

## Recovery application contract

Recovery boots from BOOT with `rdinit=/init`. The command line supplies
`eris.boot_state`, `eris.confirmed_partition`, and `eris.candidate_partition`.
The environment is authoritative; command-line values are diagnostic context.

After successfully completing the operation for the current state, recovery
must atomically save `recovery_done=1` and `boot_attempts=3`, preserving the state
and confirmed partition, then reboot. It must not acknowledge a failed or
partial operation. The operation must tolerate interruption and be safe to
retry. In particular DATA reset must recreate both the LUKS container (when
configured) and its filesystem, without relying on Linux to open an erased
LUKS header. No DATA erasure is performed in U-Boot.

`/init` mounts the virtual filesystems, then executes the C supervisor as PID 1.
The first implementation supports F2FS repair and DATA reset, including LUKS.
Rollback restoration remains unimplemented until a snapshot producer and format
are defined; that operation fails without acknowledgment and consumes attempts.
See the DM layer's recovery-init README for the implementation and host tests.

## Build artifacts

The normal WIC image depends on `recovery-rootfs-image:do_image_complete` and
copies its archive to BOOT. The normal kernel and device trees on BOOT are used
for recovery. The archive is external to the kernel. `bootz` and `booti` use
`recovery-ramfs.cpio.gz` and an address:size argument; `bootm` uses the legacy
U-Boot ramdisk `recovery-ramfs.cpio.gz.u-boot` and an address argument.

Validate on each board: persistent environment access from both Linux and
U-Boot, compatible recovery kernel drivers built in, nonoverlapping load
addresses, BOOT capacity, GPIO detection, and reset/power-cut scenarios.

Host regression checks: `python3 -m unittest discover -s tests` in the OS repo.
These mock commands and do not replace a U-Boot hush or board test.

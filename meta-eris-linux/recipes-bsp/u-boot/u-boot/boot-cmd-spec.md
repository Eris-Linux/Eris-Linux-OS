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

## Recovery application contract (implementation pending)

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

Until the application is integrated, `/init` mounts the virtual filesystems
and starts a diagnostic shell; it does not set recovery_done.

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

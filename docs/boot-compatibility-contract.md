# Boot compatibility contract for the 1.1 installation baseline

Status: source/artifact audit, 2026-11-14. This document records current
constraints and proposed release requirements. It does not claim that all
requirements are implemented or hardware-validated. See boot-memory-audit.md
for measured sizes and addresses. No boot behavior is changed by this audit.

The intended baseline is a full installation of 1.1. Subsequent releases
should update rootfs, kernel and supported DTB content without replacing
the installed U-Boot executable or requiring storage reinstallation.

## What the existing update actually changes

The updater's system.c:install_new_image() formats the inactive SYSTEM-A/B
partition with mkfs.ext2 and extracts the signed rootfs archive there. It then
schedules a trial through the persistent U-Boot environment. It does not
replace BOOT, its boot.scr, the Pi firmware/configuration/overlays, or the
recovery kernel/DTB/initramfs. A /boot file in the rootfs archive is not an
update to the separate BOOT partition.

The fixed BOOT script embeds the kernel filename, boot command, device-tree
mode/name, partition numbers, media/device, console and recovery filename at
build time. Future recipe changes do not change this installed script.

On BeagleBone the selected slot supplies /boot/am335x-boneblack.dtb. On Pi the
script instead uses fdt_addr supplied by the firmware, already including its
fixups and overlays. Shipping new DTBs in a Pi rootfs does not currently select
them at boot. The kernel must therefore remain compatible with the installed
firmware-produced tree until a different mechanism is designed and validated.

## Compatibility boundaries to establish before releasing 1.1

| Boundary | Current constraint | Requirement for future updates |
| --- | --- | --- |
| ROM, Pi firmware/EEPROM, BBB SPL | Precede U-Boot and may remain installed indefinitely | Test against the supported installed versions, not only freshly built SD images |
| CPU and executable formats | ARM32 bootm/bootz on Pi 2/BBB; ARM64 booti on Pi 3/4/5 | Retain supported architecture, image format, load/entry convention and decompression format |
| U-Boot features | Commands, parsers, compression algorithms and filesystem readers are compiled in | Image generation may use only capabilities in the installed baseline |
| Memory layout | Kernel input/output, decompressor workspace, Image memory extent, DTB, script, initramfs, framebuffer and relocated U-Boot must coexist | Define per-board budgets with growth reserves and validate normal plus recovery boot |
| BOOT protocol | Script, filenames, /chosen handling and recovery arguments are fixed in BOOT | Keep the interface stable or arrange a backwards-compatible extension path before freezing it |
| DTB and overlays | Per-slot DTB on BBB; firmware DTB on Pi | Verify new kernels against the actual tree they receive, including firmware fixups and reserved memory |
| Partitioning | MBR, BOOT=1, A=2, B=3, DATA=4; raw environment outside partitions | Preserve numbering, identification and raw reservations; budget installed A/B capacities, not only new WIC sizes |
| Root filesystem | U-Boot reads it before Linux; updater currently creates ext2; initial WIC uses ext4 | Pin filesystem features understood by installed U-Boot and all supported updaters; verify both initial and updated filesystems |
| Linux entry contract | root=PARTUUID, rootwait, ro, init=/sbin/early-init; recovery uses rdinit=/init | Retain entry points, storage/filesystem drivers built in as needed, and the expected command-line semantics |
| Persistent environment | Raw redundant MMC environment with fixed offsets, sizes and variable meanings | Keep the physical layout/format and old variable semantics; new optional variables need safe defaults |
| A/B boot state machine | BOOT script, Linux updater/boot-ok and recovery share literal states and acknowledgments | All reachable old/new component combinations must preserve attempt accounting, confirmation and recovery |
| Frozen recovery system | Old kernel, tools and get-dmk remain in BOOT | New DATA filesystem/crypto/provisioning changes must remain readable/repairable by recovery, or have a separate safe migration design |
| Persistent application data | New rootfs may change DATA before being confirmed | A failed trial must leave DATA usable by the previous rootfs, or migration must provide a recoverable transaction |
| BMP/splashscreen | BMP capabilities compiled into U-Boot; display happens before kernel loading | Freeze supported BMP formats and bound input/buffers; unsupported artwork must remain nonfatal |
| Update envelope and keys | Existing updater must authenticate, unpack and inspect the future archive | Keep supported formats/metadata/signature schemes, or introduce them through compatible intermediate updates |

## Confirmed current capabilities and gaps

The five measured physical board builds have HUSH, PART, PARTITION_UUIDS,
FDT/libfdt, FAT/EXT4 reading, SAVEENV, BMP/32bpp/RLE8 and gzip enabled. Pi 2 and
BBB also have BOOTZ; Pi 3/4/5 have BOOTI. Pi builds do not have FIT enabled.
The measured builds do not enable ZSTD, and the ARM32 builds do not enable
LZMA/LZ4. Do not rely on features enabled only in a newer local U-Boot build.

For Pi, the raw environment reserves 16 KiB per copy at 1 MiB and 2 MiB; BBB
uses 128 KiB per copy at the same offsets. BOOT starts at 4 MiB. These values
are shared by the image, U-Boot, Linux and recovery through eris-uboot-env.
They are part of the installed contract, not ordinary rootfs configuration.

The current 16 MiB Pi 2 bootm adjustment fixes the observed kernel size error
for a new U-Boot installation. It does not by itself provide large future
memory-layout reserves. The existing BOOTZ capability offers another format
choice before freezing the 1.1 baseline. Neither approach was boot-tested by
this audit. The Pi ARM64 layouts also need more growth room (see memory audit).

Changing ramdisk_addr_r or another environment value can avoid recompiling
U-Boot, but cannot be treated as automatically A/B-safe: the old fallback
kernel and frozen recovery must still work with it. Simply changing default
environment values in a new binary does not migrate already saved values.

The current update checks signature and system version, including rejection
of a previously failed version. There is no boot-capability requirement check
in the inspected installation path. Known incompatible images should be
rejected before installing/scheduling them, rather than relying on recovery.

Recovery's DATA snapshot restoration is explicitly unimplemented: rollback
currently fails without acknowledgment and eventually advances to repair.
This is a separate open issue when evaluating the complete rollback promise.

## Proposed long-term structure

Keep the installed first-stage loader small and stable. Before freezing 1.1,
decide whether it should load a constrained second-stage description/script
from the selected A/B slot. That would let boot policy evolve together with
the slot and roll back with it. The first stage would still own the trusted
partition choice, retry budget and recovery path. Delegation must preserve
those guarantees and use only commands supported by the frozen executable.
This is a design option, not an implemented mechanism in the present script.

Record a versioned, per-board installed boot contract outside the replaceable
rootfs (for example in provisioned BOOT metadata). It should identify the
actual installed artifacts/capabilities, memory budgets, filesystem profile,
partition/environment layout and recovery protocol. Do not infer installed
capabilities from the candidate rootfs's own recipe version or config.

Include corresponding requirements in authenticated image metadata. The
running updater should check that the candidate fits the installed contract
before touching the inactive partition. Requirements must be understandable
by the oldest updater allowed in the update chain; plan any evolution through
compatible intermediate releases. A missing-field compatibility rule must
not silently accept images whose boot requirements cannot be established.

## Release validation

1. Archive exact 1.1 baseline BOOT/SPL/U-Boot/firmware/recovery artifacts,
   configuration, environment layout and checksums for each supported board.
2. Build and inspect initial SD images and updater-created root filesystems.
   Check filesystem feature flags, partition capacity, fixed paths, formats,
   Image headers, decompression footprints and nonoverlapping memory budgets.
3. Boot every candidate with the frozen baseline components and representative
   saved environments. A successful boot with newly built U-Boot is not this test.
4. Exercise trial success, failed kernel/DTB/init, confirmation, fallback,
   recovery and power cuts. Include previous rootfs with new DATA and new rootfs
   with the installed old recovery system. Verify hardware identity/key access.
5. Test the actual updater path from the oldest supported rootfs, not only
   fresh installation, and reject incompatible metadata before slot writes.

Host tests can check metadata, layouts and state transitions. They cannot
certify MMC persistence, firmware/FDT behavior, decompressor relocation, device
drivers or power-cut recovery on the physical boards. QEMU build artifacts
were absent from the audit and its distinct boot chain remains unverified.

The defensible promise is that releases accepted for this installed contract
continue to boot and recover with it. An arbitrary future kernel or boot
feature cannot be guaranteed compatible without validation or, where needed,
a planned compatible migration.

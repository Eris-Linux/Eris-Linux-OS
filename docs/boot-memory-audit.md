# Boot memory audit, 2026-11-14

This is a snapshot of the existing build artifacts, not a board boot test or
a certification of firmware already installed in the field. One MiB is
1,048,576 bytes. No boot settings were changed during this audit.

## Coverage and findings

The production menu defines BeagleBone Black headless, Raspberry Pi 2/3/4/5
headless and graphical, and QEMU x86-64 headless. Both variants of each Pi have
the same deployed kernel sizes in this snapshot. The Pi kernels are 6.12.93;
the available BeagleBone kernel is still 6.6.21. No QEMU build artifacts exist,
so its boot margins cannot be measured here.

| Builds | Boot path | Kernel file/payload | Relevant margin |
| --- | --- | --- | --- |
| BeagleBone Black headless | bootz | zImage 8.538 MiB | 87.462 MiB from end of loaded zImage to DTB |
| Pi 2 headless, rebuilt U-Boot | bootm | uImage payload 8.188 MiB | 7.812 MiB below the new 16 MiB bootm limit |
| Pi 2 graphical, existing artifacts | bootm | uImage payload 8.188 MiB | **Fails the existing 8 MiB bootm limit by 197,496 bytes** |
| Pi 3 64-bit, both variants | booti | Image 28.360 MiB; memory extent 29 MiB | 8 MiB from end of relocated Image to recovery initramfs |
| Pi 4 64-bit, both variants | booti | Image 28.360 MiB; memory extent 29 MiB | 8 MiB from end of relocated Image to recovery initramfs |
| Pi 5 64-bit, both variants | booti | Image 28.485 MiB; memory extent 29.188 MiB | 7.8125 MiB from end of relocated Image to recovery initramfs |
| QEMU x86-64 headless | Not the U-Boot Pi/BBB path | No artifacts | Not verified |

These rows describe different limits and must not be interpreted as a single
common kernel size allowance. In particular, the Pi 2 bootm allowance does not
describe the space needed for the kernel's own subsequent decompression.

## Raspberry Pi ARM32

The existing Pi 2 graphical U-Boot configuration still has
`CONFIG_SYS_BOOTM_LEN=0x800000`. The rebuilt headless configuration has
`0x1000000`. The new source fragment applies to both variants at their next
U-Boot rebuild, but does not update existing binaries or SD images.

The current payload is 8,586,104 bytes and exactly matches the kernel build's
`arch/arm/boot/zImage`. It is wrapped as an uncompressed legacy uImage.
U-Boot's `boot/image.c:image_decomp()` bounds even an uncompressed copy by
`CONFIG_SYS_BOOTM_LEN`, accounting for the observed -ENOSPC (-28).

The payload is copied to 0x8000. Linux then decompresses itself: the current
`System.map` _text-to-_end extent is 20.967 MiB, ending at 0x014ff8d0.
The upstream Pi environment explicitly requires room for compressed plus
decompressed kernel data because the decompressor can relocate itself beyond
the decompressed kernel. Their combined extent is about 29.19 MiB from RAM
base, leaving about 9.81 MiB before the initramfs at 39 MiB, before additional
alignment/workspace allowances. The 16 MiB bootm setting alone is therefore
not a long-term memory layout guarantee.

Both existing Pi 2 configurations also have `CONFIG_CMD_BOOTZ=y`. A possible
way to support the current larger kernel without replacing U-Boot is to use
`zImage`/`bootz`, with the matching raw initramfs argument. This requires image
and boot script changes and hardware validation; it is not implemented by this
audit. Changing the BOOT script is still a deployment operation even though
the U-Boot executable can remain unchanged.

## Raspberry Pi ARM64

`CONFIG_SYS_BOOTM_LEN` is 64 MiB on Pi 3/5 and 16 MiB on Pi 4 (the latter comes
from meta-raspberrypi's maxsize.cfg). These values do not bound the current
uncompressed `booti` path: `cmd/booti.c` uses `booti_setup()` and the ARM64
Image header rather than `bootm_load_os()`'s decompression limit.

All current ARM64 Image headers have text_offset=0 and flags=0xa. The load
address is 0x00080000, so `arch/arm/lib/image.c:booti_setup()` aligns the
destination up to 0x00200000. The complete memory extent ends at 0x01f00000
on Pi 3/4 and 0x01f30000 on Pi 5. Recovery initramfs is loaded at 0x02700000,
leaving only 8 and 7.8125 MiB respectively. The initramfs archives themselves
are about 16.85-16.89 MiB, ending below 56 MiB.

The environment also places scriptaddr at 0x02400000 (36 MiB), PXE storage at
37 MiB and fdt_addr_r at 38 MiB. A conservative layout policy preserving all
these buffers has only 5 MiB (Pi 3/4) or 4.8125 MiB (Pi 5) below scriptaddr.
Buffer lifetimes differ, so these are conservative layout margins, not proven
booti size limits. Eris uses the firmware-provided FDT (`fdt_addr`), rather
than fdt_addr_r, on these boards. Its actual address is runtime-dependent.

## BeagleBone Black

The load address is 0x82000000, DTB at 0x88000000 and recovery initramfs at
0x88080000. The 8,952,536-byte zImage ends at 0x82889ad8, well below the DTB.
The _text-to-_end memory extent is 21.002 MiB, with the normal physical
decompression destination at 0x80008000, ending at 0x815086f8. The current
recovery archive is about 5.37 MiB. The present layout has substantial room.
The 16 MiB CONFIG_SYS_BOOTM_LEN is not the applicable size ceiling for bootz.

## Follow-up to avoid bootloader replacements

There is not a uniformly large growth margin in the current Pi layouts.
Increasing a compiled bootm limit alone cannot prevent load-buffer collisions.
Before selecting a long-term layout, consider moving the Pi initramfs and
other buffers together using existing U-Boot environment/script capabilities,
preserving compatibility with installed environments. This can avoid changing
the U-Boot executable; it still needs controlled deployment and board tests.

Build/release checks should separately enforce the supported installed
bootloader's capabilities and size limits, nonoverlapping load/relocation/
decompression regions, a defined growth reserve, and recovery boot capacity.
Check Image header memory extent (not just file length) on ARM64 and both
compressed/decompressed sizes on ARM32. Do not infer installed capabilities
from a newly rebuilt local U-Boot .config.

Measurements used `builds/build-*/tmp/deploy/images/<machine>` kernel/recovery
artifacts, the corresponding U-Boot `build/.config` and
`build/u-boot-initial-env`, ARM64 Image headers, ARM32 `System.map`, and the
actual U-Boot source trees in the build directories. Persisted device
environments can override these defaults. No deployed device was contacted.

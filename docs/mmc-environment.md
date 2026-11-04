# Redundant U-Boot environment in the MMC user area

Eris uses the U-Boot MMC backend on Raspberry Pi and BeagleBone Black. Two raw
environment slots are reserved before BOOT. They do not appear in the MBR
partition table: BOOT remains partition 1, SYSTEM-A partition 2, and the first
boot still creates SYSTEM-B and DATA as partitions 3 and 4.

| Region | Byte offset from start of medium | Raspberry Pi size | BeagleBone size |
| --- | --- | --- | --- |
| MBR / reserved boot-loader area | 0 | first 1 MiB reserved | first 1 MiB reserved |
| Environment A | `0x100000` (1 MiB) | `0x4000` (16 KiB) | `0x20000` (128 KiB) |
| Environment B | `0x200000` (2 MiB) | `0x4000` (16 KiB) | `0x20000` (128 KiB) |
| BOOT, MBR partition 1 | `0x400000` (4 MiB) | existing FAT contents | existing FAT contents, including MLO and u-boot.img |

The existing Eris BeagleBone image loads MLO and U-Boot from FAT. A future board
that places raw SPL/U-Boot elsewhere must check those ranges before adopting
this layout. The first MiB reservation alone is not a universal boot-loader
placement rule.

## Configuration and generated files

`meta-eris-linux/conf/eris-uboot-env.inc` is the common source of parameters.
`classes/eris-uboot-env.bbclass` validates alignment and overlap. The U-Boot
append generates and merges the MMC Kconfig fragment after vendor fragments,
then checks the resolved `.config` so that a silently rejected setting fails
the build. SPL keeps its existing environment policy.

`recipes-bsp/eris-uboot-env/eris-uboot-env.bb` supplies `/etc/fw_env.config` to
both normal Linux (through `u-boot-env`) and recovery (through `recovery-init`).
No second recipe owns that file. For example, the Raspberry Pi configuration is:

```
/dev/mmcblk0 0x100000 0x4000
/dev/mmcblk0 0x200000 0x4000
```

U-Boot uses `CONFIG_SYS_MMC_ENV_PART=0`, meaning the **MMC hardware user area**.
It does not mean the MBR BOOT partition. `fw_env.config` likewise names the
whole device, never `/dev/mmcblk0p1` or `/dev/mmcblk0boot0`.

SD and eMMC use the same backend and byte offsets. Device numbering must match
the board and selected boot medium, however. The current Eris profiles retain
their existing SD mapping: U-Boot MMC 0 and Linux `/dev/mmcblk0`. For an eMMC
profile, set `ERIS_UBOOT_ENV_MMC_DEVICE` and `ERIS_UBOOT_ENV_DEVICE` in machine
metadata after checking the actual numbering in U-Boot and Linux. These numbers
need not be identical. Also adapt the existing boot-script device,
`/usr/share/eris-linux/partitions`, recovery BOOT/DATA devices and the image
installation target when porting the complete system to another medium.
This change does not add automatic SD/eMMC discovery or use eMMC boot0/boot1.

The board WKS templates reserve and populate both raw slots. Existing Cooker
`WKS_FILE = '...wks'` settings remain accepted by the normal image recipe, which
selects the corresponding `.wks.in` template first. BitBake rejects overlapping
slots before constructing the image. Other profiles retain the legacy FAT
configuration while `ERIS_UBOOT_ENV_RAW` is disabled.

## Installation and update behaviour

Install a **complete newly generated `.wic` image**, including the prefix before
BOOT, on the test medium. Copying only FAT files and root filesystems is
insufficient. No migration from older FAT-environment installations is supplied.
An ordinary SYSTEM update neither initializes nor overwrites the two slots.

Both slots in a fresh installation contain `0xff`, deliberately invalid CRCs.
This also clears stale state when reinstalling a used card. U-Boot initially
uses its compiled defaults; the existing boot script's first successful
`saveenv` creates one valid copy, and `boot-ok` creates the other. Both copies
are not valid until those initial saves have succeeded. The initial bad-CRC
warning is expected. The old `/boot/uboot.env` file is no longer consulted.

Later saves alternate between the two slots using the standard U-Boot CRC and
sequence flags. A damaged copy can fall back to the other, which may contain
the preceding state. This protects the environment from a failed write without
editing FAT metadata. It does not protect BOOT files, provisioning data on FAT,
or a medium/controller failure affecting both slots. SD and eMMC still expose
logical sectors through their own controller; physical erase-block placement
and power-loss behaviour are properties of the chosen medium.

## Validation

Run the host tests from the OS repository:

```
python3 -m unittest discover -s tests -v
```

The layout test uses the checked-out Yocto WIC parser and partition layout
engine. With real libubootenv tools available, also exercise alternating writes,
corruption of either CRC, fallback and subsequent repair on temporary files:

```
ERIS_TEST_FW_PRINTENV=/path/to/fw_printenv \
ERIS_TEST_FW_SETENV=/path/to/fw_setenv \
    python3 -m unittest discover -s tests -v
```

On each board, install a complete image, allow the first boot to finish, compare
`fw_printenv` with U-Boot's `printenv`, and check both directions after changing
a disposable variable. Repeat the failed-trial rollback test. Finally, test
interrupted saves on expendable media; host tests do not establish a particular
SD/eMMC controller's power-cut guarantees. If both copies are lost, compiled
defaults cannot recover the last confirmed SYSTEM partition with certainty.

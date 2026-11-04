#!/usr/bin/env python3
"""Check MMC reservations with Yocto's WIC parser and layout engine.

Optional integration tests use the real libubootenv fw_printenv/fw_setenv tools:
ERIS_TEST_FW_PRINTENV=/path/fw_printenv ERIS_TEST_FW_SETENV=/path/fw_setenv \
    python3 -m unittest discover -s tests
All writes go to temporary ordinary files, never to a block device.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
LAYERS = ROOT.parent
sys.path[:0] = [str(LAYERS / 'poky/scripts/lib'),
                str(LAYERS / 'poky/bitbake/lib'), str(LAYERS / 'poky/meta/lib')]
from wic.ksparser import KickStart
from wic.plugins.imager.direct import PartitionedImage

CLASS = ROOT / 'meta-eris-linux/classes/eris-uboot-env.bbclass'
HELPERS = {}
source = CLASS.read_text().split('def eris_uboot_env_layout', 1)[1]
exec('def eris_uboot_env_layout' + source.split('python __anonymous()', 1)[0], HELPERS)
CONF = ROOT / 'meta-eris-linux/conf/eris-uboot-env.inc'


class Data(dict):
    def getVar(self, key):
        return self.get(key)


def settings(board):
    values = Data()
    entries = re.findall(r'^(ERIS_\w+)(?::([\w-]+))?\s*\??=\s*"([^"\n]*)"',
                         CONF.read_text(), re.M)
    for name, override, value in entries:
        if not override:
            values[name] = value
    machine_override = 'rpi' if board == 'raspberrypi' else 'beaglebone-yocto'
    for name, override, value in entries:
        if override == machine_override:
            values[name] = value
    values['ERIS_UBOOT_ENV_IMAGE'] = 'blank.bin'
    for name in ('ENV_OFFSET', 'ENV_OFFSET_REDUND', 'ENV_SIZE', 'BOOT_OFFSET'):
        values['ERIS_UBOOT_' + name + '_KIB'] = str(int(values['ERIS_UBOOT_' + name], 0) // 1024)
    return values


class LayoutTest(unittest.TestCase):
    def test_wic_offsets_and_partition_numbers(self):
        for board, size in (('raspberrypi', 16384), ('beaglebone', 131072)):
            with self.subTest(board=board), tempfile.TemporaryDirectory() as directory:
                data = settings(board)
                layout = HELPERS['eris_uboot_env_layout'](data)
                self.assertEqual(layout['SIZE'], size)
                template = ROOT / ('meta-eris-linux-%s/wic/eris-linux-sdimage-%s.wks.in' % (board, board))
                rendered = re.sub(r'\$\{(\w+)\}', lambda match: data[match[1]], template.read_text())
                wks = Path(directory) / 'test.wks'
                wks.write_text(rendered)
                parts = KickStart(str(wks)).partitions
                image = PartitionedImage(str(Path(directory) / 'test.wic'), 'msdos', parts)
                for part in parts:
                    part.size_sec = part.disk_size * 2
                image.layout_partitions()
                self.assertEqual([p.realnum for p in parts], [0, 0, 1, 2])
                self.assertEqual([p.start * 512 for p in parts[:3]],
                                 [1048576, 2097152, 4194304])
                self.assertEqual([p.size_sec * 512 for p in parts[:2]], [size, size])
                self.assertEqual([p.mountpoint for p in parts[2:]], ['/boot', '/'])

    def test_reject_unsafe_layouts(self):
        for changes in ({'OFFSET': '0'}, {'OFFSET': '512'}, {'SIZE': '0'},
                        {'SIZE': '0x200000'}, {'OFFSET_REDUND': '0x400000'},
                        {'DEVICE': '/dev/mmcblk0p1'}, {'DEVICE': '/dev/mmcblk0boot0'},
                        {'MMC_DEVICE': '-1'}):
            with self.subTest(changes=changes):
                data = settings('raspberrypi')
                data.update({'ERIS_UBOOT_ENV_' + key: value for key, value in changes.items()})
                with self.assertRaises(ValueError):
                    HELPERS['eris_uboot_env_layout'](data)

    def test_linux_and_uboot_agree(self):
        for board in ('raspberrypi', 'beaglebone'):
            data = settings(board)
            config = HELPERS['eris_uboot_env_config'](data)
            entries = [line.split() for line in config.splitlines() if not line.startswith('#')]
            self.assertEqual(entries, [[data['ERIS_UBOOT_ENV_DEVICE'], data['ERIS_UBOOT_ENV_' + key],
                                        data['ERIS_UBOOT_ENV_SIZE']]
                                       for key in ('OFFSET', 'OFFSET_REDUND')])
            fragment = HELPERS['eris_uboot_env_fragment'](data)
            self.assertIn('CONFIG_SYS_MMC_ENV_PART=0\n', fragment)
            self.assertIn('CONFIG_ENV_IS_IN_MMC=y\n', fragment)
            self.assertIn('CONFIG_SYS_REDUNDAND_ENVIRONMENT=y\n', fragment)


@unittest.skipUnless(os.environ.get('ERIS_TEST_FW_PRINTENV') and os.environ.get('ERIS_TEST_FW_SETENV'),
                     'set ERIS_TEST_FW_PRINTENV and ERIS_TEST_FW_SETENV to test libubootenv')
class RedundancyTest(unittest.TestCase):
    def test_alternating_writes_and_corrupt_copy(self):
        for board in ('raspberrypi', 'beaglebone'):
            with self.subTest(board=board), tempfile.TemporaryDirectory() as directory:
                data = settings(board)
                layout = HELPERS['eris_uboot_env_layout'](data)
                disk = Path(directory) / 'mmc.img'
                config = Path(directory) / 'fw_env.config'
                defaults = Path(directory) / 'defaults'
                defaults.write_text('test_value=defaults\n')
                config.write_text(HELPERS['eris_uboot_env_config'](data).replace(layout['DEVICE'], str(disk)))
                offsets = [layout['OFFSET'], layout['OFFSET_REDUND']]
                with disk.open('wb') as output:
                    output.truncate(layout['BOOT_OFFSET'])
                    for offset in offsets:
                        output.seek(offset)
                        output.write(b'\xff' * layout['SIZE'])

                def tool(name, *args, check=True):
                    return subprocess.run([os.environ['ERIS_TEST_FW_' + name], '-c', str(config),
                                           '-f', str(defaults), *args], check=check,
                                          capture_output=True, text=True)

                before = disk.read_bytes()
                tool('SETENV', 'test_value', 'first')
                first = disk.read_bytes()
                tool('SETENV', 'test_value', 'second')
                second = disk.read_bytes()
                changed_first = [i for i, (a, b) in enumerate(zip(before, first)) if a != b]
                changed_second = [i for i, (a, b) in enumerate(zip(first, second)) if a != b]
                first_slot = next(offset for offset in offsets if offset <= changed_first[0] < offset + layout['SIZE'])
                second_slot = next(offset for offset in offsets if offset <= changed_second[0] < offset + layout['SIZE'])
                self.assertNotEqual(first_slot, second_slot)
                for changed, slot in ((changed_first, first_slot), (changed_second, second_slot)):
                    self.assertTrue(all(slot <= index < slot + layout['SIZE'] for index in changed))
                self.assertEqual(tool('PRINTENV', '-n', 'test_value').stdout.splitlines()[-1], 'second')

                # Corrupt each CRC in turn, starting from the same valid pair.
                for slot, expected in ((first_slot, 'second'), (second_slot, 'first')):
                    disk.write_bytes(second)
                    with disk.open('r+b') as output:
                        output.seek(slot)
                        output.write(bytes([second[slot] ^ 0xff]))
                    self.assertEqual(tool('PRINTENV', '-n', 'test_value').stdout.splitlines()[-1], expected)
                    tool('SETENV', 'test_value', 'repaired')
                    self.assertEqual(tool('PRINTENV', '-n', 'test_value').stdout.splitlines()[-1], 'repaired')


if __name__ == '__main__':
    unittest.main()

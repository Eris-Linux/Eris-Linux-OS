#!/usr/bin/env python3
"""Exercise the generated boot state machine with mocked U-Boot commands.

This checks selection and persistence, not the U-Boot hush implementation.
Run with: python3 -m unittest discover -s tests
"""
import os
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
RECIPE = ROOT / 'meta-eris-linux/recipes-bsp/u-boot/u-boot_%.bbappend'
SCRIPT = ROOT / 'meta-eris-linux/recipes-bsp/u-boot/u-boot/boot.cmd.in'


def render(**overrides):
    values = dict(re.findall(r'^(ERIS_\w+)\s*\??=\s*"([^"\n]*)"', RECIPE.read_text(), re.M))
    values.update(ERIS_BOOT_MEDIA='mmc', ERIS_BLOCK_DEVICE='0',
                  ERIS_FDT_MODE='load', ERIS_FDT_IMAGE='board.dtb',
                  ERIS_GRAPHIC='0', ERIS_RECOVERY_RAMFS='recovery-ramfs.cpio.gz')
    values.update(overrides)
    result = re.sub(r'@@(ERIS_\w+)@@', lambda m: values[m[1]], SCRIPT.read_text())
    assert '@@' not in result
    return result


MOCKS = r'''
setenv() { local key="$1"; shift; printf -v "$key" '%s' "$*"; export "$key"; }
setexpr() { printf -v "$1" '%s' "$(($2 - $4))"; }
saveenv() {
    echo "SAVE:$boot_state:$boot_attempts:$confirmed_partition:$candidate_partition:$recovery_done"
    test "${FAIL_SAVE:-0}" = 0
}
load() { test "${FAIL_LOAD:-0}" = 0; }
fdt() { return 0; }
part() { setenv root_partuuid test-uuid; }
bootz() { echo "BOOT:$boot_state:$_boot_on:$_initramfs:$_boot_on_ramfs:$bootargs"; exit 0; }
reset() { echo RESET; exit 0; }
sleep() { echo HALTED; exit 0; }
'''


class BootScriptTest(unittest.TestCase):
    def run_boot(self, state='stable', attempts='3', done='0', **extra):
        env = dict(os.environ, boot_state=state, boot_attempts=attempts,
                   recovery_done=done, confirmed_partition='2', candidate_partition='3',
                   kernel_addr_r='1000', ramdisk_addr_r='2000', fdt_addr_r='3000',
                   filesize='4000')
        env.update(extra)
        return subprocess.run(['bash'], input=MOCKS + render(), env=env,
                              text=True, capture_output=True, check=True, timeout=5).stdout

    def test_stable(self):
        out = self.run_boot()
        self.assertIn('SAVE:stable:2:2:3:0', out)
        self.assertIn('BOOT:stable:2:0:-:', out)

    def test_trial(self):
        self.assertIn('BOOT:trial:3:0:-:', self.run_boot('trial'))

    def test_exhausted_system_boots(self):
        for state, next_state in [('stable', 'repair'), ('trial', 'rollback'),
                                  ('rollback', 'repair'), ('repair', 'reset')]:
            with self.subTest(state=state):
                out = self.run_boot(state, '0', '1')
                self.assertIn(f'BOOT:{next_state}:0:1:2000:4000:', out)
                self.assertIn(f'eris.boot_state={next_state}', out)

    def test_recovery_success_retries_confirmed(self):
        for state in ['rollback', 'repair', 'reset']:
            self.assertIn(f'BOOT:{state}:2:0:-:', self.run_boot(state, done='1'))

    def test_recovery_failure_budget(self):
        for state, next_state in [('rollback', 'repair'), ('repair', 'reset')]:
            out = self.run_boot(state, '0')
            self.assertIn(f'SAVE:{next_state}:2:', out)
            self.assertIn(f'BOOT:{next_state}:0:1:', out)
        self.assertIn('HALTED', self.run_boot('reset', '0'))

    def test_invalid_counter_and_candidate(self):
        self.assertIn('BOOT:repair:0:1:', self.run_boot(attempts='broken'))
        self.assertIn('BOOT:rollback:0:1:', self.run_boot('trial', candidate_partition='99'))

    def test_first_boot_and_invalid_confirmed(self):
        self.assertIn('BOOT:stable:2:0:', self.run_boot(state='', candidate_partition=''))
        self.assertIn('BOOT:stable:3:0:', self.run_boot(state=''))
        self.assertIn('HALTED', self.run_boot(confirmed_partition='99'))

    def test_bootm_ramdisk_argument(self):
        script = render(ERIS_BOOT_COMMAND='bootm')
        mocks = MOCKS.replace('bootz()', 'bootm()')
        env = dict(os.environ, boot_state='repair', boot_attempts='3',
                   recovery_done='0', confirmed_partition='2', candidate_partition='3',
                   kernel_addr_r='1000', ramdisk_addr_r='2000', fdt_addr_r='3000',
                   filesize='4000')
        out = subprocess.run(['bash'], input=mocks + script, env=env,
                             text=True, capture_output=True, check=True, timeout=5).stdout
        self.assertIn('BOOT:repair:0:1:2000: ', out)

    def test_load_failure_reboots(self):
        out = self.run_boot(FAIL_LOAD='1')
        self.assertIn('RESET', out)
        self.assertNotIn('BOOT:', out)

    def test_save_failure_halts(self):
        out = self.run_boot(FAIL_SAVE='1')
        self.assertIn('HALTED', out)
        self.assertNotIn('BOOT:', out)


if __name__ == '__main__':
    unittest.main()

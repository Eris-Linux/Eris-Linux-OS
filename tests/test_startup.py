"""Exercise startup policy without changing the host clock or contacting NTP."""
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
COMMON = ROOT / 'meta-eris-linux'
DM = ROOT.parent / 'eris-linux-dm/meta-eris-linux-dm'
NTP = COMMON / 'recipes-eris/eris-ntp/eris-ntp/eris-ntp'
NETWORK = COMMON / 'recipes-eris/eris-containers/eris-containers/eris-containers-network'


class StartupTest(unittest.TestCase):
    def run_ntp(self, results, enabled=True):
        with tempfile.TemporaryDirectory(prefix='eris-ntp-') as directory:
            root = Path(directory)
            parameters = root / 'parameters'
            parameters.write_text('ntp_enable=%s\nntp_server=test.invalid\n' %
                                  ('yes' if enabled else 'no'))
            events, log = root / 'events', root / 'ntp.log'
            tools = {
                'date': 'case "$*" in *%Y) echo 2026;; *) echo test-time;; esac',
                'hwclock': 'echo hwclock >> "$EVENTS"',
                'ping': 'echo ping >> "$EVENTS"; exit 1',
                'ntpd': '''
count=$(cat "$COUNT" 2>/dev/null || echo 0)
count=$((count + 1))
echo "$count" > "$COUNT"
echo ntpd >> "$EVENTS"
eval "result=\\$RESULT_$count"
if [ "$result" = hang ]; then
    trap '' TERM
    while :; do /bin/sleep 1; done
fi
exit "$result"
''',
                'sleep': '''
echo "sleep $1" >> "$EVENTS"
count=$(cat "$SLEEPS" 2>/dev/null || echo 0)
count=$((count + 1))
echo "$count" > "$SLEEPS"
[ "$count" -lt "$LIMIT" ] || kill -TERM "$PPID"
''',
            }
            for name, body in tools.items():
                tool = root / name
                tool.write_text('#!/bin/sh\n' + body + '\n')
                tool.chmod(0o755)
            env = dict(os.environ, PATH=str(root) + os.pathsep + os.environ['PATH'],
                       EVENTS=str(events), COUNT=str(root / 'count'),
                       SLEEPS=str(root / 'sleeps'), LIMIT=str(len(results)))
            env.update({'RESULT_%d' % i: str(value) for i, value in enumerate(results, 1)})
            # Only redirect files and shorten the real GNU timeout in this test.
            script = NTP.read_text().replace('LOG_FILE=/run/eris-ntp.log', 'LOG_FILE=' + str(log))
            script = script.replace('PARAMETERS_FILE=/etc/eris-linux/parameters',
                                    'PARAMETERS_FILE=' + str(parameters))
            script = script.replace('"/dev/rtc"', '"' + str(root / 'rtc') + '"')
            script = script.replace('NTP_QUERY_TIMEOUT=20', 'NTP_QUERY_TIMEOUT=0.15')
            script = script.replace('NTP_KILL_DELAY=2', 'NTP_KILL_DELAY=0.15')
            result = subprocess.run(['/bin/sh', '-c', script], env=env,
                                    capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, -signal.SIGTERM, result.stderr)
            return events.read_text().splitlines(), log.read_text()

    def test_initial_retries_are_fast_and_do_not_require_ping(self):
        events, log = self.run_ntp([1, 1, 0, 1])
        self.assertEqual([event for event in events if event.startswith('sleep ')],
                         ['sleep 5', 'sleep 5', 'sleep 300', 'sleep 30'])
        self.assertEqual(events.count('ntpd'), 4)
        self.assertNotIn('ping', events)
        self.assertIn('Clock synchronized', log)
        self.assertIn('[uptime=', log)

    def test_disabled_ntp_does_not_query_or_spin(self):
        events, _ = self.run_ntp([0], enabled=False)
        self.assertEqual(events, ['sleep 30'])

    def test_stalled_ntp_is_killed_and_retried(self):
        events, log = self.run_ntp(['hang'])
        self.assertIn('ntpd', events)
        self.assertIn('sleep 5', events)
        self.assertIn('NTP query failed', log)

    def test_container_marker_follows_startup_even_if_application_fails(self):
        for code in (0, 7):
            with self.subTest(code=code), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                marker = root / 'started'
                helper = root / 'containers'
                helper.write_text('#!/bin/sh\n[ ! -e "%s" ] || exit 99\nexit %d\n' % (marker, code))
                helper.chmod(0o755)
                script = NETWORK.read_text().replace('/usr/sbin/eris-containers', str(helper))
                script = script.replace('/run/eris-containers-started', str(marker))
                result = subprocess.run(['/bin/sh', '-c', script, 'network', 'start'])
                self.assertEqual(result.returncode, code)
                self.assertTrue(marker.exists())
                marker.unlink()
                subprocess.run(['/bin/sh', '-c', script, 'network', 'stop'])
                self.assertFalse(marker.exists())

    def test_management_starts_before_applications(self):
        def priority(path, variable='INITSCRIPT_PARAMS'):
            text = path.read_text()
            match = re.search(r'^' + re.escape(variable) + r'\s*=\s*"start (\d+) ', text, re.M)
            self.assertIsNotNone(match, str(path))
            return int(match[1])

        wifi = priority(COMMON / 'recipes-eris/eris-wifi-monitor/eris-wifi-monitor_1.0.bb')
        ntp = priority(COMMON / 'recipes-eris/eris-ntp/eris-ntp.bb')
        api = priority(COMMON / 'recipes-eris/eris-rest-api/eris-rest-api_1.0.bb')
        service = DM / 'recipes-eris/eris-update-service/eris-update-service_0.1.bb'
        checked = priority(service, 'INITSCRIPT_PARAMS:${PN}-notifier')
        updater = priority(service, 'INITSCRIPT_PARAMS:${PN}-updater')
        containers = COMMON / 'recipes-eris/eris-containers/eris-containers_0.1.bb'
        early = priority(containers, 'INITSCRIPT_PARAMS:${PN}-early')
        network = priority(containers, 'INITSCRIPT_PARAMS:${PN}-network')
        self.assertLess(wifi, ntp)
        self.assertLess(ntp, api)
        self.assertLess(api, checked)
        self.assertLess(checked, updater)
        self.assertLess(updater, early)
        self.assertLess(early, network)


if __name__ == '__main__':
    unittest.main()

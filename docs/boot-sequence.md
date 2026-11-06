# Startup and first Device Manager contact

SysV runlevel 5 starts the Eris services in this order:

| Priority | Service |
| --- | --- |
| 01 | networking (existing network setup) |
| 02 | D-Bus |
| 03 | Eris Wi-Fi monitor, in the background |
| 04 | Eris NTP, in the background |
| 19 | Eris REST API, including machine UUID initialization |
| 20 | Docker and the existing system services at this priority |
| 21 | boot-ok, with its existing bounded health check |
| 22 | Eris update service, in the background |
| 23 | early containers |
| 24 | network containers |

The other board services retain their existing priorities. Boot validation
precedes all Eris application containers. An unavailable network, Device
Manager or NTP server does not become a new boot-ok prerequisite.

## Time synchronization

`eris-ntp` queries the configured NTP server directly; ICMP replies are not
required. GNU timeout bounds each query to 20 seconds, with a kill-after
fallback of 2 seconds. Before the first successful synchronization, failed
queries are retried after 5 seconds. Later failures retain the 30-second delay,
and successes retain the 300-second interval. Disabled NTP is checked again
after 30 seconds.

`/run/eris-ntp.log` records startup, attempts, failures and synchronization with
timestamps from `/proc/uptime`. It also captures NTP client diagnostics. The
log rotates at 256 KiB to `.log.1`, so prolonged connection failures do not
produce an unbounded tmpfs log.

## Contact and update coordination

The updater attempts HTTPS directly, without an ICMP readiness gate. Its
existing wget limits (one try, 10-second network timeouts) remain in place.
Before the first successful contact, the delay between attempts is capped at
5 seconds; shorter configured periods are respected. After success, the
configured contact period applies. A zero period still requires an explicit
contact trigger and does not turn into automatic polling.

`/tmp/update-system.debug` records the first successful contact with system
uptime. A contact can succeed while application containers are still starting.
The successful response may update the contact policy and initialize the
provisioning baseline immediately.

Container changes and system-update requests from that response are deferred
until `/run/eris-containers-started` exists. The network-container init script
creates this marker after both startup phases have run, even if an application
failed. It records completion of startup attempts, not container health.
Subsequent contacts can apply the requested changes. System installation and
USB-update detection also wait for this marker in their own background threads;
they do not block status contacts.

Deploy the OS and DM layer changes together. A blocking container-start command
can still delay completion of SysV startup and update installation, while the
already-started updater continues status contacts. This change does not add a
timeout to individual Docker operations.

## Board validation

After installing the rebuilt image:

1. Check `/etc/rc5.d/` against the Eris priorities above.
2. Compare `/run/eris-ntp.log` and the updater's first-contact uptime. Calendar
   timestamps jump when NTP corrects the clock, so they cannot measure boot time.
3. Check that containers still start and `/run/eris-containers-started` appears.
4. Repeat with the network disconnected, then connect it: local startup must
   continue, time must synchronize and automatic contacts must recover.
5. Repeat a failed boot-ok trial: application containers must not start before
   the failed validation reboots the board.

Host tests exercise NTP failures/timeouts, service ordering, marker creation,
first-contact retries, manual contacts and deferred updates. They neither set
the host clock nor connect to a network server. Actual time-to-contact depends
on firmware, network setup, DNS, NTP and server response times; it needs board
measurements rather than a fixed startup-time promise.

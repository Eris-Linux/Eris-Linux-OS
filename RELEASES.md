# Release notes

## 1.0.3 - 2026-10-08

- Web HMI: add a new "Release Note" page (link at the end of the home page).
- Web HMI: small improvements in device display ("Last seen" field).
- Web HMI: fix an error that occured when renaming a device.

## 1.0.2 — 2026-10-06

- Containers: add debug settings through `/run/eris-debug`, with remote syslog
  forwarding and GDB attachment to an application in a selected container.
  Debug-enabled containers start after the early boot phase; stopping the
  selected container also stops its GDB server.
- Containers: import root filesystem archives (`slot.tar`) or load saved Docker
  images (`slot.dkr`) according to the available image format.
- Containers: give each slot its own persistent directory mounted at `/data`.
  Read the `data_uid` setup field to set directory ownership and permissions
  (`0700`) when a valid UID is provided.
- Debugging: add device log/GDB settings to the database, REST API and web
  interface; transmit them to devices and save them in `/run/eris-debug`.
- Container storage: add the `data-uid` field to the web interface and REST
  API, store it as `Data_UID` in the database, and send it to devices for
  inclusion in the container setup. Fix UID parsing and use a `long int`
  in the update service.
- Container updates: support full Docker images (`.dkr`) alongside root
  filesystem archives, and remove the previous `.dkr` file when installing
  a replacement container.
- Container updates: rework installation to improve status and error
  reporting; fix the slot index used to retrieve container errors.
- Web interface: improve log/GDB input fields and fix the Documentation tab.

## 1.0.1 — 2026-09-14

- Containers: add `network-needed` (internal) field.
- Containers: improve status details.
- REST API: listen for requests only from localhost.
- REST API: fix timezone reading and writing.
- REST API: fix UUID generation.
- REST API: implement `GET /api/system/kernel` API method.
- REST API: change source license from `CLOSED` to `GPL-2.0-only`.
- REST API: implement `POST /api/reboot/rollback` method.
- Liberis: implement `eris_get_system_kernel()` method.
- Liberis: rename `eris_reboot()` method to `eris_reboot_now()`.
- Liberis: fix NTP status methods.
- Device manager communication: fix explicit contact handling.

## 1.0.0 — 2026-08-24

- First public release.

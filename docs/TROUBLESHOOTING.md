# Troubleshooting and FAQ
| Symptom | Check |
|---|---|
| Missing FQBN / board unknown | Install Espressif 3.3.12 and select the exact C5 board profile |
| Header missing | Match the include to BUILD.md; install the named library and dependency versions |
| Wrong SD library selected | Remove duplicate selection from your build; use the ESP32 core SD library explicitly |
| NimBLE callback/API error | Confirm NimBLE 2.5.2 rather than an incompatible 1.x or newer API |
| Program too large | Confirm custom partitions.csv and correct slot maximum; do not shrink filesystem/OTA by guessing |
| No serial port | Check USB data cable, driver, port ownership and board download mode |
| GPS bytes but no recording | Wait for fresh fix and UTC outdoors; verify RX/TX direction and 9600 baud |
| Node connected but not scanning | Check Core GPS/session authorization, configuration and RUN ACK |
| Node fails to rejoin | Use matching firmware, check channel 6 and reboot Core/nodes together |
| SD fails | Check FAT32, contacts, supply, chip-select and correct SPI pins; back up before reformatting |
| Upload fails | Verify Wi-Fi, key format, service response, SD file and available heap; inspect history before retrying |
| Uploaded file still pending | Archive move may have failed; check service/history before duplicate submission |
| T-Dongle settings disappear after flashing | Generic boot_app0 upload can overlap NVS; use only the guarded helper |

**Can I flash C5 firmware onto S3 or classic CYD?** No. A separate verified target is required.

**Does compilation mean hardware tested?** No. Build and hardware status are separate in VERIFICATION.md.


**Where are stable downloads?** None have been published during preparation. Follow RELEASES.md and the owner-approved release notes when available.

**Why does esptool show arduino-lib-builder and another App version?** The pinned Arduino-ESP32 package supplies that image descriptor. It is separate from WardriveHub's application version strings. Use the package manifest, source hashes and firmware's displayed/serial version to identify the selected 1.7.0 target; do not infer the project version from the core's generic descriptor.

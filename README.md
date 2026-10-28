<p align="center">
  <img src="assets/wardrivehub-logo.png" alt="WardriveHub logo" width="240">
</p>

# WardriveHub

Take your wardriving kit out, collect Wi-Fi and Bluetooth observations with GPS coordinates, and bring the logs back to upload. WardriveHub runs on ESP32-C5 hardware and saves recordings locally as CSV files.

Use a **CYD Core** as a touchscreen recorder and coordinator for scanner nodes, or build a **Mini** for a standalone recorder with an OLED and button. The Core supplies GPS and storage for its nodes, so every extra scanner does not need another GPS receiver or SD card.

**Current firmware: 1.7.0.** The [prerelease downloads](https://github.com/stonedtrip/WardriveHub/releases/tag/v1.7.0-rc.1) passed compilation and offline image checks. They have **not been hardware-tested during repository preparation** and are not marked stable.

## What it does

- Collects Wi-Fi and BLE observations using each device's scanning role.
- Uses fresh GPS position and time to gate recording.
- Coordinates scanner nodes over ESP-NOW from the CYD Core.
- Saves CSV recordings to microSD on the recorders.
- Provides CSV selection, upload history and archiving.
- Uploads to WiGLE, WDGWars and Signal Frontier with your own credentials.
- Pauses scanning during uploads and restores it according to the saved session state.

## Choose your setup

| Device | What you use it for | Hardware |
|---|---|---|
| **CYD Core** | Touchscreen recorder, GPS, SD, uploads and control of scanner nodes | NM-CYD-C5 / RockBase C5; 16 MB flash, PSRAM |
| **Wi-Fi + BLE node** | Additional Wi-Fi/BLE scanning under Core control | Seeed XIAO ESP32-C5 |
| **BLE-only node** | Dedicated BLE scanning under Core control | Seeed XIAO ESP32-C5 |
| **Mini** | Standalone recorder with GPS, OLED, SD and setup/upload portal | Seeed XIAO ESP32-C5 plus peripherals |
| **T-Dongle node** | Display-equipped scanner under Core control | LilyGO T-Dongle C5; 4 MB flash, no PSRAM |

For a Core-based kit, start with the CYD, GPS and SD card; add nodes after local recording works. For a small standalone build, choose Mini. Mini operates independently and does not coordinate this node setup.

These downloads are for the listed **ESP32-C5** targets. Classic ESP32 CYD boards and ESP32-S3 boards need different firmware. Check your physical board revision and actual flash capacity before flashing.

## Parts and wiring

All devices need reliable power and a USB **data** cable for programming. CYD and Mini recording setups need a suitable NMEA GPS receiver and FAT32 microSD card. Mini also needs an SSD1306 I2C OLED at address 0x3C, SPI SD module and optional button. The XIAO scanner nodes only need their board and power; they use the Core's GPS and SD. T-Dongle uses its integrated display.

| Connection | CYD Core | Mini |
|---|---|---|
| GPS TX → board RX | GPIO4 | GPIO12 |
| Board TX → GPS RX, if used | GPIO5 | GPIO11 |
| SD SPI | SCK6, MISO2, MOSI7, CS10 | SCK8, MISO9, MOSI10, CS7 |
| OLED I2C | Integrated display | SDA23, SCL24 |
| Mini button | — | GPIO0 to GND when pressed |

Use common ground. These are chip GPIOs, which may differ from printed D labels. Check module power ratings and never feed 5 V into a 3.3 V GPIO. Use the manufacturer's connector pinout for assembled boards and integrated displays. [Full pinouts](docs/HARDWARE.md) · [Wiring diagrams](wiring/README.md)

## Install the firmware

### Download a compiled package

1. Open [Releases](https://github.com/stonedtrip/WardriveHub/releases/tag/v1.7.0-rc.1).
2. Choose the ZIP named for your device: CYD_Core, Mini, Node_Wi_Fi_and_Bluetooth, Node_Bluetooth_Only or T_Dongle_Node. The **COMPILE_ONLY_NOT_STABLE** label is intentional.
3. Extract to a short folder path and check the archive against RELEASE_SHA256SUMS.txt.
4. Open **READ_BEFORE_FLASHING.md** inside the package. Follow its exact image paths and board-specific flashing commands; keep the supplied bootloader and partition files together.
5. Back up existing recordings/configuration, connect the board, select its actual serial port and close Serial Monitor before uploading.

Windows ports usually look like COM4; Linux ports like /dev/ttyACM0 or /dev/ttyUSB0. Replace example ports with your own. GitHub's automatically generated source ZIP contains source, not ready-to-flash images.

**T-Dongle C5 needs its guarded flashing helper.** Do not use a generic Arduino upload or merged image: writing boot_app0 at 0xE000 overlaps its preserved NVS layout. Preview the operation from the extracted package root:

~~~powershell
./flashers/Flash-TDongle.ps1 -Port COM4 -WhatIf
~~~

Check the board, port and preview, then repeat without -WhatIf. On Linux, use pwsh with your actual port and an explicit -Esptool path. [Windows/Linux flashing details](docs/FLASHING.md)

### Build from source

Install Arduino IDE 2.x. Under **File → Preferences → Additional Boards Manager URLs**, add:

~~~text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
~~~

Install **esp32 by Espressif Systems 3.3.12** in Boards Manager. Open the matching sketch under firmware/ and keep ScanHelpers.h and any supplied partitions.csv beside its .ino.

| Target | Profile |
|---|---|
| CYD Core | esp32:esp32:esp32c5:FlashSize=16M,PSRAM=enabled |
| Mini and both XIAO nodes | esp32:esp32:XIAO_ESP32C5 |
| T-Dongle C5 | esp32:esp32:esp32c5:FlashSize=4M,PSRAM=disabled |

Retain the custom CYD and T-Dongle partition files. The pinned XIAO profile produces an 8 MB image header; confirm your board's capacity.

Install these pinned libraries through Library Manager or Arduino CLI:

~~~sh
arduino-cli lib install "NimBLE-Arduino@2.5.2" "TinyGPSPlus@1.0.3" "Adafruit GFX Library@1.13.0" "Adafruit BusIO@1.17.4" "Adafruit ST7735 and ST7789 Library@1.12.0" "Adafruit SSD1306@2.5.17" "XPT2046_Touchscreen@1.4.0" "LovyanGFX@1.2.35"
~~~

WiFi, ESP-NOW and SD come with the ESP32 core; CYD uses its bundled BLE API. T-Dongle uses the included LilyGo-display-library snapshot, selected by the build helper. With Arduino CLI installed and the core available, build a target from the repository root:

~~~sh
python utilities/build.py --target WARDRIVE_HUB_C5_CYD_Core
~~~

Other targets are WARDRIVE_HUB_C5_Mini, WARDRIVE_HUB_C5_Node_Wi_Fi_and_Bluetooth, WARDRIVE_HUB_C5_Node_Bluetooth_Only and WARDRIVE_HUB_C5_T_Dongle_Node. Use python3 on Linux if needed. Output goes to .validation-build/. [Complete build setup](docs/BUILD.md)

## Your first recording

### CYD Core

1. Insert the FAT32 SD card, connect the GPS and power the Core.
2. Confirm the SD card is available. For diagnostics, open Serial Monitor at **115200 baud** and check that firmware reports 1.7.0.
3. Move the GPS antenna outdoors with clear sky. Wait for fresh position **and UTC time**. Receiving serial data or seeing satellites is not the same as a valid recording fix.
4. Use the Core's session controls to start/arm recording. Confirm local recording works before adding nodes.
5. Check node join, heartbeat and run status on the **NODES** page. A node waiting for Core/GPS authorization is not necessarily faulty.
6. Stop recording before removing the card or disconnecting power. Inspect a short saved CSV before relying on a longer field session.

The touchscreen navigation has **DASH**, **NODES**, **LIVE**, **CSV** and **SET**. Use DASH for the session overview, NODES for scanner status, CSV for recordings and uploads, and SET for settings and diagnostics.

### Mini

Power Mini with its GPS, OLED and SD connected. Its recording flow waits for fresh GPS position/time. The display shows the setup/upload portal's connection details; use the generated password shown by the device.

Button actions happen **on release**:

| Press length | Action |
|---|---|
| Short press | Change display page |
| More than 650 ms, below 2.5 seconds | Pause/resume an armed session |
| More than 2.5 seconds, below 6 seconds | Start/stop a session |
| More than 6 seconds | Toggle the portal |

Release within the intended interval so a longer action does not replace a shorter one. A manual stop cancels pending automatic start. Close the portal before expecting normal scanning.

## Add scanner nodes

The CYD Core, both XIAO node variants and T-Dongle node in this release share the ENOW protocol. Nodes use the Core's GPS and storage and require its authorization; they are not standalone recorders.

1. Flash the matching firmware and power the nodes near the Core.
2. Wait for the Core's GPS readiness and enable its capture session.
3. Check join/configuration, heartbeat and run acknowledgement on NODES or serial diagnostics.
4. Confirm observations reach the Core's CSV, not just a node's discovery counter.

ESP-NOW rendezvous uses **channel 6**; observation channel hopping is separate. If joining fails, stop the session, reboot the matching Core/nodes together and inspect reconnect diagnostics. [Pairing details](docs/NODES.md)

## Save and upload your logs

CYD keeps pending CSVs under **/wardrive hub/csv/** and archives under **/wardrive hub/csv/Uploaded CSVs/**. Mini creates files such as `/WDH_<time>_<sequence>.csv` and maintains its own portal/history.

For uploads, copy [the blank example](config/examples/uploadcfg.txt) to **uploadcfg.txt at the SD root** and edit it locally. Use KEY=value lines without shell quotes. Leave unused services empty:

~~~ini
WIFI_SSID=
WIFI_PASSWORD=
WIGLE_API_KEY=
WDGWARS_API_KEY=
SIGNAL_FRONTIER_API_KEY=
~~~

| Service | Credential |
|---|---|
| WiGLE | Your account's encoded Basic API credential; Base64 does not make it safe to share |
| WDGWars | Your API key, sent as X-API-Key |
| Signal Frontier | Your device/API key; optional SIGNAL_FRONTIER_HOST overrides signalfrontier.org |

Stop recording, select a closed CSV and destination(s) in the recorder's upload interface, then check the response and upload history. Upload work pauses scanning to free radio/memory resources; recovery follows the saved session state. A session you stopped first will not necessarily resume as an active recording.

Back up pending CSVs, archived CSVs and upload_history.csv. An archive reflects acceptance by the selected destination(s), not every supported service. If upload succeeds but archiving fails, check history before retrying so you do not submit the same recording again.

WiGLE/WDGWars test actions can submit a real selected CSV; use a recording you intend to upload. Signal Frontier's device check uses /api/v1/device/me. Never put keys or populated configuration in an issue. [Upload reference](docs/UPLOADS.md)

## Common questions

| What you see | What to check |
|---|---|
| GPS waits indoors | Move outdoors; check GPS TX → board RX, common ground and 9600-baud GPS UART. Wait for position and time. |
| Discoveries increase but CSV rows do not | Check fresh GPS, an active recording session and SD storage. Deduplication also makes counts differ. |
| Node joins but does not scan | Check Core capture authorization, GPS state and run acknowledgement. |
| Node does not join | Confirm matching C5 firmware, power nearby, then restart Core/nodes and inspect diagnostics. |
| Upload fails | Check Wi-Fi, service credentials, connectivity and the returned response. Keep the CSV. |
| Uploaded file remains pending | Inspect history: its upload may have succeeded but its archive move failed. |
| No serial port or upload fails | Try a USB data cable, select the actual port, close Serial Monitor and follow the board's BOOT/RESET procedure. |
| Mini is not scanning | Check session state, GPS freshness and whether the portal is still open. |

[More troubleshooting](docs/TROUBLESHOOTING.md)

## Firmware and verification

All five application targets are **1.7.0** and passed compilation with Arduino CLI 1.5.1 and Arduino-ESP32 3.3.12. Published firmware source/support files remain unchanged from the selected originals. Compilation and image checks do not establish physical radio, GPS, SD, display or upload behavior.

Before relying on a kit, check a short recording, GPS loss/recovery, node reconnects, SD errors and upload pause/resume on your hardware. Linux build/upload instructions have not been exercised on Linux during preparation. [Verification results](docs/VERIFICATION.md) · [Version history](CHANGELOG.md)

## Project files and further reading

| Folder | Contents |
|---|---|
| firmware/ | Device source, individual READMEs and target manifests |
| flashers/ | Build and guarded flashing helpers |
| config/examples/ | Blank upload configuration |
| wiring/ | Code-derived connection diagrams |
| docs/ | Detailed builds, setup, uploads, troubleshooting and verification |
| utilities/ | Build utility and safeguards |
| third_party/ | Dependency snapshot and notices |

Use WardriveHub where you have permission to collect and upload observations. CSVs can reveal locations and network identifiers. The preserved uploader uses TLS without validating server certificates; read [SECURITY.md](SECURITY.md) before using keys on an untrusted network.

Original credits and notices are retained. No project license has been selected and third-party provenance review remains incomplete. [Licensing](docs/LICENSING.md) · [Third-party notices](THIRD_PARTY_NOTICES.md) · [Contributing](CONTRIBUTING.md)

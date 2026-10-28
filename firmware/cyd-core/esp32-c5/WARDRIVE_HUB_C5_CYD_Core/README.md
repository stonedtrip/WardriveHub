# NM-CYD-C5 / RockBase ESP32-C5 Core

Firmware **1.7.0**, source target `WARDRIVE_HUB_C5_CYD_Core`.

Coordinates the ENOW node cluster, performs local Wi-Fi/BLE observation, owns GPS/session authorization and records SD CSVs. Includes touchscreen CSV management, service uploads and application-only SD update. Requires the 16 MB board and PSRAM profile; not classic CYD 2.8-inch.

## Parts and first setup
Use the assembled board, external NMEA GPS, FAT32 microSD and reliable USB power. GPS TX connects to GPIO4 RX; optional GPS RX to GPIO5 TX. Wait for fresh GPS/time, arm a session, then check node run acknowledgements. Stop capture before removing SD.

## Build and flash
Dependencies: Adafruit GFX, BusIO, ST7735/ST7789, XPT2046_Touchscreen, TinyGPSPlus; BLE comes from Espressif core.

From repository root:
```
python utilities/build.py --target WARDRIVE_HUB_C5_CYD_Core
```
Keep the .ino, ScanHelpers.h and any partitions.csv together. See [exact profiles and versions](../../../../docs/BUILD.md), [wiring](../../../../docs/HARDWARE.md), and [Windows/Linux flashing](../../../../docs/FLASHING.md).

## Verify operation
Inspect startup/version diagnostics at 115200 baud, then test the actual display/buttons/radio/GPS/SD functions fitted to this device. Test lost GPS or lost Core, recovery, and pause/resume before relying on data. [Compile evidence](../../../../docs/VERIFICATION.md) is separate from hardware status. No hardware test was performed during repository preparation.

Source and support files match the selected input byte-for-byte. See [setup](../../../../docs/SETUP.md), [node protocol differences](../../../../docs/NODES.md), [uploads](../../../../docs/UPLOADS.md), [troubleshooting](../../../../docs/TROUBLESHOOTING.md) and [license concerns](../../../../docs/LICENSING.md).

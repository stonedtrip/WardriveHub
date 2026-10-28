# XIAO ESP32-C5 Mini standalone

Firmware **1.7.0**, source target `WARDRIVE_HUB_C5_Mini`.

Independent Wi-Fi/BLE recorder with SSD1306 OLED, GPS gating, SD CSVs and setup/upload portal. It does not require or replace the CYD coordinator.

## Parts and first setup
Connect GPS TX to GPIO12 RX and optional RX to GPIO11 TX; OLED SDA23/SCL24 at 0x3C; SD SCK8/MOSI10/MISO9/CS7; button GPIO0. Uses 9600-baud GPS and fresh position plus UTC before automatic capture. Portal credentials appear on the display and are generated locally. CSVs use /WDH_*.csv; archives /Uploaded CSVs; history /upload_history.csv.

## Build and flash
Dependencies: Adafruit GFX, BusIO, SSD1306, TinyGPSPlus and NimBLE-Arduino.

From repository root:
```
python utilities/build.py --target WARDRIVE_HUB_C5_Mini
```
Keep the .ino, ScanHelpers.h and any partitions.csv together. See [exact profiles and versions](../../../../docs/BUILD.md), [wiring](../../../../docs/HARDWARE.md), and [Windows/Linux flashing](../../../../docs/FLASHING.md).

## Verify operation
Inspect startup/version diagnostics at 115200 baud, then test the actual display/buttons/radio/GPS/SD functions fitted to this device. Test lost GPS or lost Core, recovery, and pause/resume before relying on data. [Compile evidence](../../../../docs/VERIFICATION.md) is separate from hardware status. No hardware test was performed during repository preparation.

Source and support files match the selected input byte-for-byte. See [setup](../../../../docs/SETUP.md), [node protocol differences](../../../../docs/NODES.md), [uploads](../../../../docs/UPLOADS.md), [troubleshooting](../../../../docs/TROUBLESHOOTING.md) and [license concerns](../../../../docs/LICENSING.md).

# XIAO ESP32-C5 BLE-only node

Firmware **1.7.0**, source target `WARDRIVE_HUB_C5_Node_Bluetooth_Only`.

Scans BLE advertisements under CYD authorization and sends records via ENOW. BLE-only describes observation capture; the Wi-Fi radio still carries ESP-NOW transport. Does not record to SD or upload to external services.

## Parts and first setup
USB-powered XIAO ESP32-C5; no separate GPS, display or SD is required. Power near the CYD, wait for join/configuration and start capture at the Core after GPS lock. Check heartbeats and run acknowledgement.

## Build and flash
Dependencies: NimBLE-Arduino 2.5.2; Wi-Fi and ESP-NOW from the core.

From repository root:
```
python utilities/build.py --target WARDRIVE_HUB_C5_Node_Bluetooth_Only
```
Keep the .ino, ScanHelpers.h and any partitions.csv together. See [exact profiles and versions](../../../../../docs/BUILD.md), [wiring](../../../../../docs/HARDWARE.md), and [Windows/Linux flashing](../../../../../docs/FLASHING.md).

## Verify operation
Inspect startup/version diagnostics at 115200 baud, then test the actual display/buttons/radio/GPS/SD functions fitted to this device. Test lost GPS or lost Core, recovery, and pause/resume before relying on data. [Compile evidence](../../../../../docs/VERIFICATION.md) is separate from hardware status. No hardware test was performed during repository preparation.

Source and support files match the selected input byte-for-byte. See [setup](../../../../../docs/SETUP.md), [node protocol differences](../../../../../docs/NODES.md), [uploads](../../../../../docs/UPLOADS.md), [troubleshooting](../../../../../docs/TROUBLESHOOTING.md) and [license concerns](../../../../../docs/LICENSING.md).

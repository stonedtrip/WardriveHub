# LilyGO T-Dongle ESP32-C5 display node

Firmware **1.7.0**, source target `WARDRIVE_HUB_C5_T_Dongle_Node`.

CYD-controlled Wi-Fi/BLE ENOW scanner with integrated display and button GPIO28. The selected source explicitly has no Android/app-node mode. A historical 2.5.0 AppNode is a different branch.

## Parts and first setup
Use actual C5 T-Dongle hardware, 4 MB flash, PSRAM disabled and the preserved factory partition table. No external GPS/SD is needed for this node. Build with LILYGO_LGFX_USE_T_DONGLE_C5 (already set in source). Use only the guarded flashing helper: generic boot_app0 at 0xE000 overlaps NVS.

## Build and flash
Dependencies: NimBLE-Arduino 2.5.2, LilyGo-display-library 0.1.0 and LovyanGFX 1.2.35. Exact LilyGO source provenance is still pending.

From repository root:
```
python utilities/build.py --target WARDRIVE_HUB_C5_T_Dongle_Node
```
Keep the .ino, ScanHelpers.h and any partitions.csv together. See [exact profiles and versions](../../../../../docs/BUILD.md), [wiring](../../../../../docs/HARDWARE.md), and [Windows/Linux flashing](../../../../../docs/FLASHING.md).

## Verify operation
Inspect startup/version diagnostics at 115200 baud, then test the actual display/buttons/radio/GPS/SD functions fitted to this device. Test lost GPS or lost Core, recovery, and pause/resume before relying on data. [Compile evidence](../../../../../docs/VERIFICATION.md) is separate from hardware status. No hardware test was performed during repository preparation.

Source and support files match the selected input byte-for-byte. See [setup](../../../../../docs/SETUP.md), [node protocol differences](../../../../../docs/NODES.md), [uploads](../../../../../docs/UPLOADS.md), [troubleshooting](../../../../../docs/TROUBLESHOOTING.md) and [license concerns](../../../../../docs/LICENSING.md).

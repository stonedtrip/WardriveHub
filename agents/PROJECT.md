# Inspected project context

Source: [stonedtrip/WardriveHub](https://github.com/stonedtrip/WardriveHub), main inspected 2026-11-04. This is a documentation/tree snapshot, not a firmware code audit. Recheck the target checkout before implementation.

## Device and source map

| Device | Source subtree | Role |
|---|---|---|
| CYD Core | firmware/cyd-core/esp32-c5/ | Recorder, touchscreen, GPS/SD, node coordinator |
| Mini | firmware/mini/esp32-c5/ | Independent recorder, OLED/button, portal |
| Wi-Fi/BLE node | firmware/nodes/esp32-c5/wifi-ble/ | Core-controlled scanner |
| BLE node | firmware/nodes/esp32-c5/ble-only/ | Core-controlled BLE scanner |
| T-Dongle | firmware/nodes/esp32-c5/t-dongle/ | Core-controlled scanner with display |

Firmware targets and release metadata live under firmware/. Build tooling lives in utilities/ and flashers/. Consult docs/NODES.md for coordination, docs/UPLOADS.md for service behavior, docs/HARDWARE.md and wiring/ for connections, and docs/FLASHING.md for flashing.

## Baseline and known limits

The inspected [README](https://github.com/stonedtrip/WardriveHub/blob/main/README.md) identifies firmware 1.7.0 with prerelease status. [Verification](https://github.com/stonedtrip/WardriveHub/blob/main/docs/VERIFICATION.md) records compilation of five targets and explicitly leaves physical hardware validation outstanding. It also distinguishes host-side build safeguards from firmware behavior.

The build baseline uses Arduino-ESP32 3.3.12. Read [BUILD.md](https://github.com/stonedtrip/WardriveHub/blob/main/docs/BUILD.md) for dependency pins and board profiles; do not substitute a generic ESP32 profile. T-Dongle has a board-specific guarded flashing path. Credentials and recordings need careful handling, and [SECURITY.md](https://github.com/stonedtrip/WardriveHub/blob/main/SECURITY.md) documents existing certificate-validation limitations. Licensing and dependency provenance remain documented open items; preserve notices and consult docs/LICENSING.md before distribution changes.

## Useful commands in the actual checkout

Build one target:

```text
python utilities/build.py --target WARDRIVE_HUB_C5_CYD_Core
```

Other target IDs:

```text
WARDRIVE_HUB_C5_Mini
WARDRIVE_HUB_C5_Node_Wi_Fi_and_Bluetooth
WARDRIVE_HUB_C5_Node_Bluetooth_Only
WARDRIVE_HUB_C5_T_Dongle_Node
```

Build all with PowerShell: `./flashers/build.ps1`. Inspect utilities/test_build.py for the existing host safeguard tests and their invocation. Run these commands from the repository root after installing the documented build dependencies.

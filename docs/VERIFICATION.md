# Firmware verification

Verification date: 2026-10-27. Arduino CLI 1.5.1, Espressif Arduino-ESP32 3.3.12, RISC-V GCC 14.2.0; installed dependency versions match BUILD.md. Initial sandbox attempts failed with a Windows DLL relocation error. Actual builds were retried outside the sandbox in isolated staging directories. No board was flashed or accessed.

| Target | Version | Compile | Hardware |
|---|---|---|---|
| WARDRIVE_HUB_C5_CYD_Core | 1.7.0 | PASS | NOT TESTED |
| WARDRIVE_HUB_C5_Node_Wi_Fi_and_Bluetooth | 1.7.0 | PASS | NOT TESTED |
| WARDRIVE_HUB_C5_Node_Bluetooth_Only | 1.7.0 | PASS | NOT TESTED |
| WARDRIVE_HUB_C5_Mini | 1.7.0 | PASS | NOT TESTED |
| WARDRIVE_HUB_C5_T_Dongle_Node | 1.7.0 | PASS | NOT TESTED |

Details and application hashes: [BUILD_RESULTS.json](BUILD_RESULTS.json). Source/support integrity: [SHA256SUMS](../firmware/SHA256SUMS.txt). Raw logs remain local because they contain personal paths. Supplied historical validation claims were not substituted for new results.

All 12 selected firmware/support files remain byte-identical, preserving existing scan logic, GPS gating, ESP-NOW transport, upload maintenance and CSV management. That identity is evidence of preservation, not proof of runtime correctness. Custom generated partition entries are compared to the preserved CSVs for successful builds.

Four computer-only tests pass for the Python T-Dongle build safeguards: correct manifest, failed-build stale-manifest removal, oversized application rejection and changed-partition rejection. PowerShell helpers parse successfully. These mock tests do not simulate radios, flash, GPS, SD, service APIs or the full Arduino runtime.

The manual GitHub Actions workflow covers all five targets and does not publish artifacts or releases. It has not been run on GitHub during preparation. Linux build/upload instructions have not been exercised on Linux.

Before stable release: bench-test GPS acquisition/loss/recovery, Core authorization and pause/resume, both bands/BLE where supported, join/rejoin/reboot, SD full/removal/failure, CSV deduplication/history/archive failure, service responses, display/buttons and any SD OTA path. Record board revision and actual test results separately.

Final candidate builds apply GCC file-prefix maps to strip local user, Arduino and staging paths from diagnostics. Earlier unredacted binary packages were withheld locally. Firmware source bytes were not edited for this cleanup. Offline esptool image checks validate chip identity, image checksum and validation hash separately from compilation.

The staged T-Dongle helper also passed a WhatIf preview against the real generated images and safe manifest. No uploader or serial device was invoked.


# Flash firmware on Windows or Linux
Back up CSVs and configuration. Build the exact board target first and read the compile result; a successful compile does not prove hardware operation. Connect a USB data cable, select the correct COM port (Windows) or /dev/ttyACM* or /dev/ttyUSB* (Linux). Close Serial Monitor before upload.

For CYD, Mini and XIAO nodes, open the correct .ino in Arduino IDE, select the profile from BUILD.md, verify and upload. Retain the custom partition table where present. Use board-specific BOOT/RESET instructions if the port does not enter download mode. Do not erase the whole flash as a routine troubleshooting step.

CLI example, XIAO Wi-Fi/BLE node (substitute your actual port):
```
arduino-cli upload --fqbn esp32:esp32:XIAO_ESP32C5 --port COM4 --input-dir .validation-build/WARDRIVE_HUB_C5_Node_Wi_Fi_and_Bluetooth
```
On Linux replace COM4 with /dev/ttyACM0 after confirming it with arduino-cli board list. If access is denied, inspect device permissions and your distribution's serial-access group; log out/in after a group change. Do not run arbitrary downloaded flashers as administrator.

## T-Dongle C5: required special path
Generic Arduino 3.3.12 uploads can write boot_app0 at 0xE000, overlapping the preserved T-Dongle NVS layout. Do not use generic IDE upload, generic merged images, full-chip erase or a guessed offset list for this target.

Build with the repository helper, which removes unsafe merged output and writes a safe-flash-manifest.json. Windows PowerShell preview:
```
./flashers/Flash-TDongle.ps1 -Port COM4 -WhatIf
```
Then repeat without -WhatIf only after checking the selected board and port. Pass -Esptool with the executable path if necessary. The helper validates hashes, sizes and the partition CSV, then writes only bootloader at 0x2000, partition table at 0x8000 and application at 0x10000.

On Linux, install PowerShell (pwsh) and use the same guarded helper with explicit tool and port paths:
```
pwsh -File flashers/Flash-TDongle.ps1 -Port /dev/ttyACM0 -Esptool /absolute/path/to/esptool -WhatIf
```
The Linux upload path has not been executed during preparation. Verify esptool compatibility with the preserved helper before removing -WhatIf.

## CYD SD update
Build the CYD target. Copy only WARDRIVE_HUB_C5_CYD_Core.ino.bin to SD root as update.bin and follow the device prompt. Do not use merged, bootloader or partition images as update.bin. The supplied code validates image/chip/OTA space; this active update path does not enforce filename-version ordering. The candidate's successful-install archive name is installed_v1.7.0.bin. Preserve recordings and verify the device after reboot.

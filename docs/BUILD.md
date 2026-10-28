# Install Arduino and build
Install Arduino IDE 2.x from https://www.arduino.cc/en/software/. In File > Preferences add the Espressif board index below. In Boards Manager install **esp32 by Espressif Systems 3.3.12**. Select your actual board and serial port. Keep every sketch folder name matched to its .ino and keep ScanHelpers.h and any partitions.csv beside it.

Board index: https://espressif.github.io/arduino-esp32/package_esp32_index.json

Library Manager versions from the supplied build record:

| Library | Version | Devices |
|---|---|---|
| NimBLE-Arduino | 2.5.2 | nodes, Mini, T-Dongle |
| TinyGPSPlus | 1.0.3 | CYD, Mini |
| Adafruit GFX Library | 1.13.0 | CYD, Mini |
| Adafruit BusIO | 1.17.4 | Adafruit dependency |
| Adafruit ST7735 and ST7789 Library | 1.12.0 | CYD |
| Adafruit SSD1306 | 2.5.17 | Mini |
| XPT2046_Touchscreen | registry 1.4.0 (installed metadata 1.4) | CYD |
| LilyGo-display-library | 0.1.0 | T-Dongle |
| LovyanGFX | 1.2.35 | T-Dongle dependency |

CYD uses the ESP32 core's bundled BLE API. WiFi, ESP-NOW, SD, LittleFS, Preferences and Update are supplied by the core. Avoid unrelated libraries with the same names, especially SD.

## CLI setup (Windows PowerShell or Linux shell)
Install Arduino CLI from https://docs.arduino.cc/arduino-cli/installation/ and put it on PATH. From the repository root:
```
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "NimBLE-Arduino@2.5.2" "TinyGPSPlus@1.0.3" "Adafruit GFX Library@1.13.0" "Adafruit BusIO@1.17.4" "Adafruit ST7735 and ST7789 Library@1.12.0" "Adafruit SSD1306@2.5.17" "XPT2046_Touchscreen@1.4.0" "LovyanGFX@1.2.35"
```
The exact local LilyGo-display-library 0.1.0 source snapshot is retained under third_party/ with its LICENSE. The build utilities select it explicitly for T-Dongle; install LovyanGFX 1.2.35. Its upstream commit remains unknown; see third_party/README.md.

Build one target with Python 3 (use python3 on Linux if needed):
```
python utilities/build.py --target WARDRIVE_HUB_C5_CYD_Core
```
Or build all using PowerShell:
```
./flashers/build.ps1
```
The Windows helper accepts -ArduinoCli, -ConfigFile and -Esp32Libraries overrides. The Python utility accepts --cli and --config. Output is local under .validation-build, excluded from Git.

Both helpers apply GCC file-prefix maps to local home, Arduino and repository directories so diagnostic strings do not embed personal build paths. Firmware sources remain unchanged. If compiling directly in Arduino IDE, inspect resulting binary strings before public redistribution: local filesystem paths can otherwise appear in compiled assertions.

## Exact profiles and partitions
CYD: esp32:esp32:esp32c5:FlashSize=16M,PSRAM=enabled; T-Dongle: esp32:esp32:esp32c5:FlashSize=4M,PSRAM=disabled. Both XIAO nodes and Mini: esp32:esp32:XIAO_ESP32C5 defaults from 3.3.12. Do not override these with an arbitrary partition menu setting.

CYD and T-Dongle supply custom partitions.csv. CYD application limit is 3145728 bytes; T-Dongle 3145728. XIAO builds use the board package's default layout (supplied record: 3342336 byte application slot). Check actual flash capacity and generated partitions before upload. Arduino's reported maximum can differ from custom slot size; the build utilities set the appropriate maximum for the custom layouts.

The pinned XIAO ESP32-C5 profile generates an 8 MB flash image header. This is a compile-profile fact; confirm your physical board's capacity rather than assuming every revision is identical.

Official references: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html and https://docs.arduino.cc/arduino-cli/getting-started/ . Local source profiles remain authoritative for this candidate.

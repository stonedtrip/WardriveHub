<p align="center">
  <img src="assets/wardrivehub-logo.png" alt="WardriveHub logo" width="240">
</p>

# WardriveHub

GPS-aware Wi-Fi and BLE observation firmware for ESP32-C5 handhelds and scanner nodes. WardriveHub combines local CSV recording, a CYD-controlled scanning cluster, and upload tools for WiGLE, WDGWars and Signal Frontier.

**Publication candidate: 1.7.0. Hardware testing has not been performed during repository preparation.** Review [verification](docs/VERIFICATION.md) and [licensing](docs/LICENSING.md) before distribution. No stable binary release is published by this preparation.

## Downloads

[GitHub Releases](https://github.com/stonedtrip/WardriveHub/releases) contains the board-specific publication candidates. Read each package manifest and flashing instructions. Candidates are compile-verified; hardware testing is pending.

## Choose your device

| Hardware | Firmware | Role | Profile |
|---|---|---|---|
| NM-CYD-C5 / RockBase C5 color touchscreen | CYD Core 1.7.0 | GPS coordinator, local scanning, SD CSVs, uploads | C5, 16 MB, PSRAM |
| Seeed XIAO ESP32-C5 | Wi-Fi/BLE node 1.7.0 | CYD-managed Wi-Fi and BLE | XIAO_ESP32C5 |
| Seeed XIAO ESP32-C5 | BLE-only node 1.7.0 | CYD-managed BLE | XIAO_ESP32C5 |
| Seeed XIAO ESP32-C5 with OLED/GPS/SD | Mini 1.7.0 | Standalone recorder and upload portal | XIAO_ESP32C5 |
| LilyGO T-Dongle C5 | T-Dongle node 1.7.0 | CYD-managed display node | C5, 4 MB, no PSRAM |

These are source targets, not confirmation of every hardware revision. ESP32-S3 nodes and classic ESP32 CYD 2.8-inch devices are not interchangeable with C5 images. Historical classic CYD sources were discovered; their selection is pending separate review.

## Start here

1. Identify your board using [hardware and wiring](docs/HARDWARE.md).
2. Follow [Arduino installation and builds](docs/BUILD.md).
3. Follow [Windows/Linux flashing](docs/FLASHING.md). T-Dongle has a special flashing restriction.
4. Complete [first setup and GPS](docs/SETUP.md), then [node pairing](docs/NODES.md).
5. Read [CSV storage and uploads](docs/UPLOADS.md) and [troubleshooting](docs/TROUBLESHOOTING.md).

Firmware remains byte-identical to the selected input. Each device has its own README under firmware/. Source, local compiled images, and release archives are kept separately. Check [release instructions](docs/RELEASES.md) before downloading or building a package. No verified device screenshots were supplied for this candidate; none are fabricated.

Use only where you have permission to collect and upload observations. Recordings can expose locations and network identifiers. Existing HTTPS compatibility code does not authenticate remote certificates; see [security](SECURITY.md).

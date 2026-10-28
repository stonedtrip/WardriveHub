# Hardware and parts
Confirm the model, chip, flash size and revision before building. C5 and S3 binaries are incompatible. A classic ESP32 CYD 2.8-inch board also needs different firmware. No verified S3 target was found in the initial WardriveHub inventory.

Required for all devices: correct board, reliable USB data cable, adequate regulated power, computer with Arduino setup. Recorders need a suitable NMEA GPS and FAT32 microSD. Mini additionally needs an SSD1306 I2C OLED (0x3C), SD SPI module and optional button. Mesh nodes use the Core's GPS/SD and do not need their own.

| Device | Source pin assignments |
|---|---|
| CYD C5 | GPS RX=4, TX=5; SPI SCK=6 MISO=2 MOSI=7; SD CS=10; TFT CS=23 DC=24 BL=25; touch CS=1 |
| Mini XIAO C5 | GPS RX=12 TX=11; OLED SDA=23 SCL=24; SD CS=7 MOSI=10 MISO=9 SCK=8; button=0 |
| XIAO Wi-Fi/BLE or BLE-only node | USB power; no external GPS/SD required |
| T-Dongle C5 | Retained display profile: SCK=6 MOSI=2 DC=3 CS=10 RST=1 BL=0; button=28; integrated ST7735S 80x160 panel |

GPIO numbers are chip GPIOs, not necessarily the silk-screen D numbers. These are code-derived wiring references, not measured electrical schematics. Physical board revisions and connector voltages remain unverified. For assembled CYD/T-Dongle boards use the board manufacturer's connector pinout; do not rewire integrated displays based on this table.

See [wiring diagrams](../wiring/README.md). Never feed 5 V into a 3.3 V GPIO. Supply peripherals according to their actual module ratings and use common ground.

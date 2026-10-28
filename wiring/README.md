# Code-derived wiring diagrams
These diagrams cover signal direction. Confirm module power ratings and connector labels before applying power. Integrated board wiring is described in ../docs/HARDWARE.md and is not a replacement for manufacturer schematics.

```mermaid
flowchart LR
 GPS[GPS NMEA module] -->|TX to GPIO4 RX| CYD[CYD C5 Core]
 CYD -->|GPIO5 TX to RX optional| GPS
 GPS ---|common GND| CYD
```

```mermaid
flowchart LR
 GPS[GPS] -->|TX to GPIO12 RX| MINI[XIAO C5 Mini]
 MINI -->|GPIO11 TX to RX optional| GPS
 MINI ---|SDA23 SCL24| OLED[SSD1306 0x3C]
 MINI ---|SCK8 MOSI10 MISO9 CS7| SD[microSD SPI]
 MINI ---|GPIO0 to GND when pressed| BTN[Button]
```


All connected modules share GND. GPS serial configuration is 9600 baud in the selected recorder sources. Mesh nodes connect wirelessly to CYD, not to GPS UART wires. No verified photographs or manufacturer wiring drawings are redistributed here.

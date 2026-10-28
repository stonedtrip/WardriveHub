# First power-up and GPS
1. Verify board model and wiring, install the correct firmware and insert a FAT32 SD card where supported.
2. For uploads, place your private uploadcfg.txt at SD root. Leave API fields empty until you have accounts and keys.
3. Open Serial Monitor at 115200. Confirm the firmware reports 1.7.0, SD is available and GPS UART is receiving data. Move the GPS antenna outdoors with clear sky; indoor fixes can fail.
4. Wait for fresh position and UTC time. CYD owns the mesh recording session and only authorizes node capture while its GPS-dependent session is active. Mini starts after fresh GPS position/time; its standalone behavior is distinct from node pairing.
5. Start/arm the CYD session using the device UI. Check both local recording and node status. Safely stop recording before removing SD or power.

GPS satellites/fix/time are distinct. NMEA bytes without a valid fix are not sufficient. On loss of a fresh lock, observe safe gating and recovery before trusting recordings. Discovery counts and recorded CSV rows may differ because coordinates/time gate recording and deduplication suppresses repeats.

Mini exposes its setup/upload portal and shows connection details on its display; use its generated password. Do not assume a fixed shared password. See each device README for its role.

Before field use, check GPS loss/recovery, both Wi-Fi bands where supported, BLE, display/buttons, SD errors, node reconnects and upload pause/resume on your actual hardware. No physical test is implied by this guide.

Mini button actions occur on release: short press changes display page; hold more than 650 ms pauses/resumes an armed session; more than 2.5 seconds starts/stops a session; more than 6 seconds toggles the portal. These are the source thresholds, so release within the intended interval. A manual stop cancels pending automatic start. Close the portal before expecting normal scanning.

# Source audit and preservation

The five selected .ino files match RELEASE.json. Every copied ScanHelpers.h and partitions.csv also matches the original bytes; firmware/SHA256SUMS.txt records all 12 selected input files. No firmware statement, pin, protocol definition, partition or application version was changed during repository preparation.

| Area | Source evidence | Implication |
|---|---|---|
| Core upload pause | CYD beginUploadMaintenance saves session/node state, stops capture/promiscuous scan, sends stop control and temporarily deinitializes BLE | Preserve the paired maintenance/recovery path; computer mocks cannot prove radio recovery |
| Mini GPS gating | Mini record/sweep paths check gpsFresh, gpsTimeFresh, sessionArmed and paused; gpsStartPending handles first valid position/time | Startup discovery counts are not proof of a location-valid CSV record |
| Core-controlled nodes | Node scanning checks runEnabled, hubKnown and peer state; heartbeat loss triggers rejoin | Do not add an autonomous fallback to conceal pairing failures |
| CSV archives | CYD cydArchiveUploadedCsv, Mini archiveMiniCsv are distinct implementations | Retain each device's paths/history; archive acceptance is destination-specific |
| T-Dongle partitions | NVS is 0x9000–0xEFFF; app is factory at 0x10000; no OTA-data partition | Generic 0xE000 boot_app0 would overlap NVS; guarded helper is required |
| CYD SD installer | Active update.bin path validates image/chip/space separately from filename-version helper | Do not promise active installer downgrade prevention based on filename tests |
| TLS | Recorder upload paths call setInsecure | Server identity is not authenticated; preserved behavior is documented in SECURITY.md |

Repository preparation changes helper path resolution, applies compiler file-prefix maps for binary privacy, and adds documentation. Diagnostic path redaction does not edit firmware source. The retained local LilyGO dependency snapshot also matches its installed source bytes and carries its original license. The Python build utility is new and has separate computer-only guard tests.

Scope limits: this is a static preservation and packaging audit with compilation evidence reported separately. It is not a complete formal proof, RF benchmark, electrical validation, service API certification or hardware test. Historical claims and prior mocked tests supplied in the archive are not treated as freshly executed results.

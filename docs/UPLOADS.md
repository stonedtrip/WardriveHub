# CSVs, configuration and uploads
Create uploadcfg.txt at SD root from config/examples/uploadcfg.txt. Lines are KEY=value, without shell quotes. Empty values disable/unconfigure the corresponding setting.

| Key | Value |
|---|---|
| WIFI_SSID | Your local upload network name |
| WIFI_PASSWORD | Its password |
| WIGLE_API_KEY | WiGLE API encoded Basic credential from your own account; source accepts an optional Basic prefix |
| WDGWARS_API_KEY | WDGWars API key (sent as X-API-Key) |
| SIGNAL_FRONTIER_API_KEY | Signal Frontier device/API key |
| SIGNAL_FRONTIER_HOST | Optional HTTPS hostname; default signalfrontier.org; do not include a URL scheme/path |

Obtain account credentials using each service's own account/API settings. Services may change their enrollment pages; this guide describes the preserved firmware contract rather than promising current service acceptance. Source endpoints include api.wigle.net /api/v2/file/upload and wdgwars.pl /api/upload-csv. Signal Frontier's test uses GET /api/v1/device/me. WiGLE credentials are secrets even when Base64 encoded. Never paste them into issues.

CYD CSV Manager separates pending files under /wardrive hub/csv/ from archives under /wardrive hub/csv/Uploaded CSVs/ and recognizes older archive locations. Mini creates /WDH_<time>_<sequence>.csv and maintains its own portal/history. Device-specific paths remain in the source and have not been unified.

Select a closed CSV and destination(s), inspect the result and per-service history. Upload work pauses scanning as implemented to release radio/memory resources and resumes according to the saved session state. Verify this on hardware. Avoid removing the card during a transfer or archive move.

An archived file means the selected destination(s) accepted it; it does not prove acceptance by all services. A failed archive can leave a successful upload in the pending list, so inspect history before retrying. Back up both pending and archived CSVs. Do not delete a local copy solely because a network request was attempted.

WiGLE/WDGWars test actions may send a real selected CSV; they are not always read-only authentication checks. Use a recording you intend to submit. Signal Frontier's device/me test validates the configured key via its API. Never fabricate observations to test a service.

Existing uploads use TLS compatibility mode without certificate verification. This behavior is preserved and documented in SECURITY.md. No online uploads or key tests were performed during preparation.

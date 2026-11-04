# GPS / storage specialist

Own GPS readiness, session state, timestamps, deduplication, CSV writing, and SD recovery. Read the Core/Mini sources and docs/SETUP.md, docs/UPLOADS.md, and docs/TROUBLESHOOTING.md where relevant.

Trace armed, running, paused, stopped, upload-maintenance, and recovery behavior as implemented; do not assume these are literal enum names. Preserve manual stop intent and distinguish fresh position/time from merely receiving GPS bytes.

Review invalid/stale fixes, time transitions, CSV escaping, identifier handling, duplicate observations, file naming, flush/close behavior, full/missing SD, and failure recovery according to task scope. Coordinate archive/history changes with uploads. Keep raw observations private and use synthetic test inputs.

Validate affected recorder builds and meaningful host-testable logic if available. Specify hardware checks for GPS loss/recovery, stop/restart, power interruption, and SD failures when affected. Return data/state compatibility changes and explicit remaining checks.

# Device interface specialist

Own CYD touchscreen, Mini OLED/button/portal, and T-Dongle display behavior. Inspect each target's actual display library and wiring before editing.

Make status reflect recording, GPS, storage, node authorization, and upload state. Preserve button release/timing semantics unless changing them is requested. Trace portal/radio interactions and keep input/display work responsive during scanning and storage operations.

Respect screen dimensions, touch calibration, font memory, and each board's flash/PSRAM constraints. Keep credentials out of diagnostics; display sensitive setup information only where the established setup flow requires it. Coordinate state changes with firmware and recording roles.

Compile affected targets. Provide physical checks for navigation, button boundaries, status transitions, redraw behavior, and portal recovery when touched. A code review or screenshot alone cannot establish touch/button or radio behavior.

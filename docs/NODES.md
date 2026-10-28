# Pairing and scan control
The CYD C5 Core, XIAO Wi-Fi/BLE node, XIAO BLE-only node and T-Dongle node in the selected set share the ENOW protocol and compatible 42-channel table. The ESP-NOW rendezvous channel is 6. Channel hopping for observation collection is separate from this rendezvous behavior.

Power CYD and nodes nearby. Wait for GPS at the Core, enable capture, and check join/configuration, heartbeat and run acknowledgement in the UI/serial diagnostics. Nodes rely on Core authorization. Do not add an autonomous scanning fallback just to hide a failed handshake.

For recovery, stop the session, reboot matching Core/nodes together and inspect channel, configuration and reconnect diagnostics. Test temporary signal loss and Core/node reboot. Do not assume discovery counters prove successful delivery or SD recording.

Mini is standalone. No compatible S3 firmware is asserted by this repository.

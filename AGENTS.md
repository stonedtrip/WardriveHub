# WardriveHub agent instructions

## Project
Target repository: https://github.com/stonedtrip/WardriveHub.
WardriveHub is an ESP32-C5 firmware project for GPS-tagged Wi-Fi/BLE observations, local CSV recording, coordinated scanner nodes, and service uploads. This repository contains the device firmware, build tooling, and shared agent instructions.

Before implementation, read the target checkout's README.md, CONTRIBUTING.md, relevant firmware README, and applicable docs. Verify its current revision and local changes. Read agents/PROJECT.md for the inspected baseline and agents/README.md for role selection. Repository source and current manifests take precedence over this snapshot.

## Working method
- Act as the lead by default. Use the smallest set of specialist briefs needed; one agent can perform multiple roles sequentially.
- Specialist briefs are task instructions, not automatically registered agents. Do not spawn workers unless the user explicitly requests delegation or applicable instructions authorize it.
- For substantial changes, record scope, acceptance criteria, affected targets, and validation in a task using agents/templates/TASK.md. Keep routine fixes lightweight.
- Preserve existing user changes. Keep edits focused; avoid unrelated cleanup or dependency upgrades.
- Trace cross-device behavior before changing a packet, shared helper, session transition, or output format. ScanHelpers.h exists in multiple sketch directories: compare copies rather than assuming they are interchangeable.
- With authorized parallel work, assign disjoint file ownership and an integration owner. Serialize edits to shared contracts, manifests, partition layouts, and release metadata.
- Record implementation evidence and unresolved work with agents/templates/HANDOFF.md when transferring work or ending a substantial session.

## Firmware constraints
- Keep sketch folder/.ino names aligned and retain adjacent support files.
- Respect board-specific flash, PSRAM, partition, wiring, and library settings from firmware/targets.json and docs/BUILD.md.
- Preserve fresh GPS position/time gating, explicit Core run authorization, and manual stop intent unless the task specifically changes them.
- Distinguish ESP-NOW rendezvous from observation channel hopping. Review Core and every affected node together for protocol changes.
- Preserve observations and upload history through failures. Model upload acceptance separately from successful archive movement and retry behavior.
- Never replace the guarded T-Dongle flashing flow with generic uploads. Review docs/FLASHING.md before flash-related changes.
- Keep credentials, populated configuration, raw location/MAC data, and personal build paths out of committed artifacts and reports. Use synthetic fixtures.
- Treat TLS certificate verification as a known review area; do not introduce additional insecure defaults. Consult SECURITY.md for current behavior.

## Validation and completion
Use the checkout's documented build commands. Compile affected targets; include peers when changing protocol/shared behavior and all targets for build-wide changes. Run relevant existing helper tests for build-tool changes. Record commands, outcomes, and any unavailable tools honestly.

Separate source review, host tests, compilation, image/partition inspection, and physical hardware results. A successful compile does not validate GPS, radio, SD, UI, or uploads. Do not describe a build as hardware-tested or stable without recorded evidence.

Do authorized local implementation and verification autonomously. Flashing a physical board, submitting real recordings, publishing releases, or deploying changes requires authorization covering that action. Documentation work alone does not require running firmware builds.

Final reports state what changed, affected targets, checks performed, and remaining hardware checks. Do not claim a remote branch or repository was updated when only this local kit changed.

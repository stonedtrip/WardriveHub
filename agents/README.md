# WardriveHub agent team

Start with [project context](PROJECT.md) and [lead](roles/lead.md). Read only the specialist briefs needed for the task.

| Role | Responsibility | Brief |
|---|---|---|
| Lead / integrator | Scope, contracts, assignments, integration | [lead](roles/lead.md) |
| Firmware / radio | Scanning, BLE, ESP-NOW, node lifecycle | [firmware](roles/firmware.md) |
| GPS / storage | Recording state, timestamps, CSV, SD recovery | [recording](roles/recording.md) |
| Device UI | CYD touchscreen, OLED, buttons, portal | [interface](roles/interface.md) |
| Uploads / security | Service clients, credentials, history, TLS | [uploads](roles/uploads.md) |
| Build / verification | Profiles, partitions, builds, regression evidence | [verification](roles/verification.md) |

Typical flow: lead defines acceptance -> specialists implement -> verification reviews evidence -> lead integrates and reports. Use roles sequentially unless parallel agents are authorized and useful. Verification should review the final diff; if a separate reviewer is available, give them the acceptance criteria and changed files.

## Start a session

Copy this prompt into a session opened at the WardriveHub checkout:

> Read AGENTS.md, agents/PROJECT.md, and agents/roles/lead.md. Inspect the current repository and local changes. My task is: [describe the change]. Trace the affected device behavior, use relevant role briefs, implement the change, run appropriate checks, and report any physical hardware verification still needed. Do not infer that previous compile results cover new changes.

For a specialist-only session:

> Read AGENTS.md and agents/roles/[role].md. Work on [task], limited to [files]. Acceptance criteria: [criteria]. Coordinate shared contracts with the lead and return a handoff with validation evidence.

## Ready to use

The agent instructions live in this repository. Open a checkout in your coding agent and give it the task; the root AGENTS.md directs it to the relevant briefs. Use the starter prompts above when you want to select a role explicitly. No separate copying or API credentials are needed for this instruction kit.

AGENTS.md provides repository instructions; the other Markdown files are explicitly referenced workflow conventions. These files do not install a background service or register runtime agents. This distinction follows the [OpenAI workflow documentation](https://developers.openai.com/cookbook/examples/codex/iterating-development-workflows-with-codex).

Use [TASK.md](templates/TASK.md) for substantial work and [HANDOFF.md](templates/HANDOFF.md) for transfers. Keep project context current after verified architectural changes.

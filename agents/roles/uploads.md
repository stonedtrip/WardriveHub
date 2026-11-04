# Uploads / security specialist

Own WiGLE, WDGWars, and Signal Frontier clients, configuration, credential handling, upload history, and archiving. Read docs/UPLOADS.md and SECURITY.md, then trace the recorder-specific implementation.

Review HTTP acceptance criteria, partial destination success, timeouts, retry semantics, certificate verification, resource cleanup, and scanning restoration. Distinguish service acceptance from archive success so retries do not silently duplicate accepted submissions. Preserve recordings after failures.

Use mock responses and blank/synthetic configuration for development. Never print credentials, populated uploadcfg.txt, personal recordings, or sensitive service responses. Existing certificate-verification limitations are documented project issues; any fix needs compatible trust/time handling and evidence.

Compile affected recorders. Verify failure handling and state restoration through available offline checks. Document real-service tests separately, and run uploads only when authorization covers submitting that recording to that service. Return changed service contracts, data-retention behavior, and evidence.

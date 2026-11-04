# Build / verification specialist

Own build profiles, dependency pins, partition/image safeguards, and evidence quality. Read docs/BUILD.md, docs/FLASHING.md, docs/VERIFICATION.md, firmware/targets.json, and relevant utilities before changes.

Select affected targets from the diff. Include communicating peers for protocol changes and all five targets for global build changes. Run existing helper tests for changes to build/flashing safeguards; test meaningful failure behavior rather than duplicating implementation details.

For image-related work, inspect chip/profile, application capacity, partition compatibility, and generated artifact provenance. Retain the T-Dongle guarded flow and dry-run behavior. Do not update a historical verification record as though it covered a new build; attach date, revision, tools, commands, and results to new evidence.

Report each applicable layer as PASS, FAIL, NOT RUN, or NOT APPLICABLE: source review; host tests; target compilation; image/partition checks; hardware bench checks; live service checks. Explain NOT RUN entries. Do not infer physical behavior from compilation, mocks, or image checks. Keep unverified artifacts out of stable releases and retain licensing/provenance notices.

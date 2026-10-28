# Release preparation and downloads
The public source repository is [stonedtrip/WardriveHub](https://github.com/stonedtrip/WardriveHub). Download board-specific candidates from [GitHub Releases](https://github.com/stonedtrip/WardriveHub/releases). Publication was approved by the owner; no project license has been selected.

After approval, download only an asset whose device, chip, flash size and version match your board. Read its manifest and flashing instructions and verify SHA-256 hashes. Source archives do not contain ready-to-flash firmware. Compile-only candidates must not be described as hardware-tested stable releases.

For each approved device package include the exact source and support files, compiled application, board/FQBN, dependency versions, partition table and required bootloader/partition binaries where relevant; include licenses/notices required by the linked dependencies, hashes, compile log summary and explicit hardware-test status. T-Dongle packages must exclude generic merged images and boot_app0 and include the guarded helper/manifest. CYD SD updates use the application image only.

The 1.7.0 packages are compile-only prerelease candidates, with hardware testing pending. Third-party licensing/provenance review remains unfinished and original notices are retained. Placeholder images are never packaged. Historical unverified local .bin files are not promoted into releases.

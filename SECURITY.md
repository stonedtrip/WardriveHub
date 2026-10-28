# Security
Do not post API keys, passwords, populated uploadcfg.txt, SD images, raw CSVs or personal location data in issues. Report a vulnerability privately through the repository owner's GitHub profile until a private advisory channel is configured.

The preserved CYD/Mini upload code uses setInsecure(): traffic is encrypted but the server certificate is not validated. This permits impersonation by a network attacker. The behavior was not silently changed during repository preparation. Review it before relying on API credentials on untrusted networks.

Mini generates a portal password and stores it in Preferences. SD uploadcfg.txt stores secrets as plaintext. Restrict physical access and rotate exposed credentials. Private IP addresses used by a device portal are defaults, not home-network credentials.

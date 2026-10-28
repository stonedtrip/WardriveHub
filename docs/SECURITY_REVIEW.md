# Candidate security review

Scanned 81 text/source files plus the decoded logo image, excluding Git metadata and Python bytecode; no archives or firmware binaries are included in the source candidate. Pattern categories cover populated upload configuration, common token forms, private keys, literal password/key assignments, email addresses, private IPv4 addresses and personal Windows paths. Source configuration initialization and generated portal password behavior were also reviewed.

Findings in the retained source: intentional device network defaults. The five Mini IP references are the intentional device portal default 192.168.4.1, retained to preserve working behavior. No populated user credential, token, personal path or private key was identified in the candidate. Blank configuration values are intentional placeholders.

The broader local text/archive scan examined 1080 file/member entries and found legacy credential-like values and extensive personal compiler paths. Those historical files are excluded; they were not copied into this candidate. Legacy binaries/CAD/images were withheld rather than certified free of embedded data. Original files remain unchanged.

Static scanning cannot certify the absence of every possible secret or personal detail. Review source/diffs and repeat scanning before publishing changed files or a Git history. Original Git history was not imported. The .gitignore excludes populated uploadcfg.txt, recordings/history, local state and generated binaries/archives. The only uploadcfg.txt exception is config/examples/uploadcfg.txt, which has empty values.

Binary review: local compiler paths were found in the first compiled images, so those packages were withheld. Final candidate builds use GCC file-prefix maps. Printable strings in the rebuilt images are scanned separately for personal paths, common tokens and complete PEM private-key blocks. Standalone PEM parser labels are not private-key material. Hardware behavior and complete transitive license clearance remain separate review items.

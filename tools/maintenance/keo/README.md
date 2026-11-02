# KEO temporary maintenance checkpoint

The local reviewed routing-OFF correction is preserved as a reproducible git
patch here because its separate checkout still has the original author's
repository as origin. Do not push our maintenance branch to the author's repo.
No new public fork/upload has been made by this checkpoint; GitHub CLI currently
has no authenticated account for creating that fork.

Checkout: D:/DEV/Kenshi/forks/KEO; branch leopard; commit 13953b03cb2f8df1638a9194d58467372f922262.
The patch's parent is the earlier local ed366bb commit (upstream e712fa6).
Candidate DLL remains uninstalled; only the source correction is backed up.

Root and independent fresh test each pass436PROD+436DEV and5compiled mutants;
full upstream host gate is not green (inherited harness failures documented in
patch). No promise of full vanilla behavior, GOG support, spawn fix or MCM port.

Independent evidence: research/keo-routing-review-20261102,
research/keo-spawn-audit-20261102, research/keo-gog-audit-20261102.
The reports separate proven native mechanisms from unreproduced user symptoms.
GOG must not be enabled by removing the two gates: literal Steam/manual
addresses require real GOG byte/layout proof, beyond named SDK address tables.

User policy: temporary bug/compatibility maintenance only, credit original
jacobericson / Jakematt2004 and GPLv3; withdraw the maintenance fork if original
author returns and continues development. Public release follows normal
three-language/MCM/logging/Nexus-before-Steam requirements.


Further reviewed local foundation commit49b1550 is preserved separately in
mcm-foundation-20261102.patch. It adds the native/host translation boundary and
requested-settings capture before runtime overrides, without MCM UI/save/apply.
Reviewed local commit6e15b3a (2026-11-03) is preserved in
0001-Integrate-MCM-controls-request-local-path-budgets-an.patch.
It integrates the MCM requested-settings/atomic-file adapter, request-local
player path budgets and optional producer-gated diagnostics with three MO
catalogues. Root and separate reviewers verified native DEV/PROD builds,
126 host suites, boundary/corruption/lifecycle and compiled regression fixtures.
No GOG/spawn/full-routing acceptance or public release is implied. Current
candidate PROD SHA256 e155894936c0fceaac3813bd88448b64f5dd896a047a66bd369bad3fa51c4324.
Local game installation and UI/FPS/save-load acceptance are recorded separately;
this patch remains remotely backed up without pushing to the original author.

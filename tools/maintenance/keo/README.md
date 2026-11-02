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
Later MCM and path-budget changes are separately uncommitted review candidates;
do not infer they are installed or ready for release from this checkpoint.

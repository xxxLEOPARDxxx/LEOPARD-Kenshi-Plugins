#pragma once
// Install only after the economic GUI tick hook succeeds. Native physiology
// stays intact; this disables the ordinary NPC minimum only for our record.
bool DfInstallNaturalHunger();

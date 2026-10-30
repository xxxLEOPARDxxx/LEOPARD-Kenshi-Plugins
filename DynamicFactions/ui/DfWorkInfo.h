#pragma once
class Character;
// Stabilize native work-stat text only for managed, actually operating members.
bool DfInstallWorkInfo(bool (*managed)(Character*));

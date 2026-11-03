# First live fine-viewer regression findings

The first game run of candidate `c89c0076b035cb4dced628adcada44c8f6284b4c4db260f0c6f620e0e805cf94`
failed to start fine production. The 125.451-second plan requested a center pair
at 3m spacing, followed at 125.460 by `current regional material does not match
verified base pins`. Subsequent logs showed admitted=0, stored=0 and legacy
coarse residency. This was not successful fine-detail acceptance.

Read-only inspection of the actual loaded process established that the PE
profile and both geography code pins matched, and actual biome records had
the expected type. The imported KenshiLib `ou` pointed to the static GameWorld
at image+0x21330b0; image+0x2133960 contained a different object. SDK
`GameWorld.h` identifies the difference, +0x8b0, as `ZoneManager* zoneMgr`.
The new adapter had incorrectly required these two different object types to
have equal addresses. The old working probe never required that equality.

The corrected adapter separately captures and verifies
GameWorld -> ZoneManager -> BiomeMap -> 32x32 table. The profiled engine slot
must equal GameWorld.zoneMgr; the caller's world scope still equals GameWorld.
Before/after identity checks, code profile, source slots, tree and complete
regional material SHA checks remain required. A compile-time offset assertion
uses the actual SDK. The old equality mutant fails the focused production
root-chain test; the correction passes 11 checks each /Od and /O2, including
stale zone/map/table and unreadable-slot refusal.

Separately, fine admission deleted offscreen fixed overview tiles. The stable
viewer had retained all sixteen. Fast pan/zoom then exposed ordinary-map strips
until the missing overview could be read and shaded again. The correction keeps
all sixteen fixed overview tiles and retires only additional detail windows.
The full requested-layer/retirement budget still decides fine admission; lack
of space defers refinement instead of sacrificing coverage. The extracted
production residency tail passed 254 focused /Od and /O2 checks, including 128
pan/zoom layouts. These controls do not replace the user's visual check.

The replacement DLL is 1,086,976 bytes, SHA-256
`310cf75904dd662c416d1105932b2da7e863d29cdd1a8d6990be029c7b5d325e`.
The parent repeated the native loader check (85 imports, exact startup and MCM
exports), confirmed the installed hash and preserved the user's INI hash.
No full atlas recapture or new atlas copy was needed. Live retry remains the
next acceptance step; neither image quality nor all city geometry is declared
complete by this report.

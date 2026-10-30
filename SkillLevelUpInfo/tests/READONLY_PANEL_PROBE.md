# One-run native XP panel telemetry

`readonly_panel_probe.py` accepts the audited872b/df789 candidates and the
corrected-style candidate SHA-256 `1487b01b6194345ead383a1708565ce5b5c6e2669fc0c472fb1332ed6185ee43`,
each with its matching linker-map timestamp and loaded-module header. The verified
header gate permits only OptionalHeader.ImageBase to contain the actual module
base instead of the on-disk preferred base; all other 1024 header bytes must
match exactly, and SizeOfImage must equal the module inventory. It enumerates
modules only inside the explicitly supplied PID, never enumerates processes,
and never searches for another PID when the target exits. No game functions,
injection, memory writes, widget-tree search or installed-file changes occur.

Run immediately after Root confirms the new live PID, substituting that PID:

```powershell
& C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/readonly_panel_probe.py --pid 12345 --seconds 10 --interval .25
```

Defaults use `build/SkillLevelUpInfo/SkillLevelUpInfo.dll` and the adjacent
matching `.map`. JSON goes only to this module's ignored `tests/_generated`.
Optional `--out`, `--dll`, `--map` take explicit local paths. Bounds: 30 seconds,
120 samples, 8 MiB total reads, 256 KiB per sample, 24 target-panel categories,
128 rows/category, 512 player registry entries, 64 pointer-map search levels.
A disappearing process stops inspection and saves already collected data.
Samples with changing map topology are marked as errors; identity stability
is recorded before/after reading. Read-only sampling cannot acquire the game's
lock or guarantee an atomic frame.

For an old872b process, explicitly supply its retained pair
`--dll tests/_generated/SkillLevelUpInfo-872b.dll --map tests/_generated/SkillLevelUpInfo-872b.map`
using absolute paths. The loaded DLL must equal the supplied candidate, not
merely any candidate on the audited list; the two maps are not interchangeable.

Each successful sample includes:

- `g_diag*` capacity/row/layout fields and status; PanelBars, MaxRows,
  recent lifetime, load/seen generation.
- Current full owner handle, panel/root/category, slot IDs/count and the
  five row-skill/value cache entries.
- Selected owner's real `g_seen` Snapshot, all active skills plus explicit
  Dexterity/Martial arts/Athletics: value, pending, last gain, age, recent flag;
  these three skills also include their raw float reference value and matching
  pointer-index binding owner/skill/statIndex/token checks.
- Every category in the current native target panel, with native key order,
  row key/type, actual name/value/progress coordinates, bottom, parent and
  visibility. Only direct parents of those bounded native rows are read.
- Distinct panel/category/owner identities observed across the polling period.
  This is not an exhaustive list of all calls between samples.

`probe_layout.cpp` was compiled independently with VC10/NDEBUG and the real
SDK. It makes no game calls and does not build/change the plugin. Measured:
hand32, Skill28, Snapshot1344, refs1040, live1336, pending1337; VC10 node value24,
pair<hand,Snapshot>.second32, therefore Snapshot at node+56. Native panel's
inner map is at outer-node+32 and row pointer at inner-node+64. VC10 string
size40, buffer0, length16, capacity24. Eleven offline parser tests verify these
layouts, native map ordering, selected-owner skill decoding and binding lookup:

```powershell
& C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/test_readonly_probe.py -v
```

PID26720 initially failed the earlier exact-header comparison. A bounded 1024-byte
read proved that only bytes 0x14a..0x14d inside ImageBase differed: preferred
0x180000000 became the actual loaded base0x7ffe7f480000. Both disk DLLs had the
exact872b hash, timestamp0x6ae4814c matched the map, SizeOfImage0x27000 matched
the module inventory, and every other header byte matched. The corrected gate
is restricted to this proven field change; tests reject wrong bases, image
sizes and timestamp changes. Raw header and byte diff remain in ignored
`_generated/runtime-26720-header.bin` / `runtime-26720-header-diff.json`.

The current probe also reads actual direct/client text fonts, text-region/view
size, alignment/shadow, divider coordinates and the two cached measurements.
The loaded-font-profile flag is available in the new matching map. Eleven offline
parser checks include the getter-verified native EditText offsets and composite
font capture. For the previous df789 process use its explicitly retained
`SkillLevelUpInfo-df789.dll/.map` pair; default paths refer to the new candidate.

Corrected192-byte measurement cache: logical/rendered WidgetRefs at0/24,
text/font pointers48/56, actual UString caption64, face128, numeric fields168,
shadow188, logical EditBox word-wrap189, rendered EditBox word-wrap190. These offsets were measured using
production VC10/SDK sizeof/offsetof. Source EditText::getCaption returns+0x70,
verified in the actual installed getter; UTF16 caption backing is bounded.

## Are existing Debug logs sufficient?

Current logs expose settings, transition status and final geometry/capacity/
row count. They do **not** identify the panel/category/owner, selected slot IDs,
which skill was active/recent or its last gain, nor actual progress coordinates
after another native layout pass. The probe supplies those missing values
without changing the DLL. Existing state-change logging can repeat if different
panels alternate; Debug should remain limited to Root's one requested run.

If polling misses rapid panel switches and no cause is proved, a minimal next
instrumentation proposal is a fixed 16-context table or 32-event scalar ring
recorded from the real Panel calls, keyed by panel/root/category/full handle.
Store generation, call counts, bottom/pitch/capacity, slots, recent bitmask and
the three relevant skill values/timestamps; allocate no widgets or strings in
the capture path. Expose it through the matching map and/or one rate-limited
Debug summary per second. This is a proposal only: no such production change
or rebuild has been made, and no XP threshold/lifetime change is proposed.

# One-town passive loaded-building census

Synchronous render-thread debug helper, not a loaded-building renderer. The caller
supplies one already resolved live primary Town and expected SceneManager/world
generation. OFF must clear outputs and return before any query. Verified/profile
admission precedes native reads. Shared production sequencing caps keys and visited
movable entries at4096 each, selected ordinary Entities16, subentities16 each, declaration elements16 each and
material records64 total. Names copy into fixed128-byte fields; overflow refuses
that record with explicit counters. All outputs are scalar values/copies. One
optional borrowed Entity is published only after ownership, scene and generation
revalidation and may only be consumed immediately in the same serialized call.

Read the embedded TownBuildingsManager and its existing map/floor FastArrays;
never insert/create/load/convert/toggle native objects. Validate actual ordinary
Entity type plus physical owner and existing typed scene registry before material
reads. No texture getters/loads, no COM/resource ownership, no GPU work or worker.
Inspect actual SubEntity material and first existing technique/pass program names,
record alternative technique/pass counts explicitly rather than claiming the first
pass is active. Copy vertex declaration/ranges only from read-only SubMesh LOD0.
Unsupported/missing/overbudget entries are reported, never invented or truncated
without an incomplete flag. World/registry/source changes abort publication.

Root owns nearest already-loaded-town selection (this helper reads only its supplied
Town), Debug-only cadence/logging, runtime fixture and later material adapter.
Use RED actual shared sequencing tests, then bounded abnormal/cancellation fixtures,
compiled functional mutants, native ABI/profile generation and actual adapter compile.
Independent review before root integration; no main/build/install edits here.

Physical owner comparisons share4096 initial reads and4096 publication rechecks.
Exhaustion is explicit. Caller cadence is owned by root (Debug-only, at most5s).

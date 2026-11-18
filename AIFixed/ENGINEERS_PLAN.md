# Engineer coordination implementation plan - 2026-11-18

Goal: improve player construction logistics while native goal election retains
authority over combat, explicit orders, higher jobs and mod AI packages.

Architecture: narrowly scoped native finder/action adapters and bounded pure
policies. The adapters run on the native calling thread. Only identity/quantity
metadata is synchronized; no background worker touches game objects or native
static scratch lists. All unsupported binaries or ambiguous ownership fall back
to native behavior. This is development, not a release or gameplay validation.

## Scope and acceptance

- Player generic engineering only; no edits to AI packages or permanent jobs.
- Native source/theft/path/transfer and inventory-capacity checks stay effective.
- Account for materials already carried/in transit. Commit leases only for an
  elected construction goal or an exact current native acquisition action.
- Reject stale ground sources already in inventories. Never follow a carrier
  as a valid ground-material source; never take from that carrier.
- Replace the construction storage batch of two with bounded useful batches.
- Prefer finishing productive machines in actual placement order when known;
  walls remain native and parallel. Existing unrecorded chronology is unknown.
- Return only proven construction surplus, at the original engineer job rank;
  do not insert urgent orders, generic resource dumping or strip personal stock.
- Bound all maps, proposal lists and maintenance work; no periodic world scan.
- Load/reset/disable/preemption invalidate reservations without dereferencing
  old-world objects. Every native call remains outside metadata locks.
- RU/EN/ZH settings, opt-in default, restart flag for actual hook installation.
- Offline tests/build/review cannot establish measured FPS or gameplay results.

## Evidence

Private authenticated native audits under D:/DEV/Kenshi/research:
engineer-materials-audit-20261118, engineer-construction-audit-20261118,
engineer-job-priority-audit-20261118. Steam 1.0.65 EXE SHA256:
504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e.
Header address comments are not executable contracts. Nonexported hooks require
exact binary/callsite/vtable admission. No GOG native validation is claimed.

## Tasks

- [x] Authenticate selection/commit/source/transfer/election boundaries.
- [x] Run unchanged policy baseline (217 +20 +59 assertions).
- [x] EngineerMaterialsPolicy.h / EngineerMaterialsRuntime.h: RED/GREEN bounded
  claims, committed-goal ownership, stale-source filtering, batch and pickup
  adapters, observed owned-surplus credits. Test merged/preexisting stock,
  scarcity, concurrent claims, cancellation, capacity and generation reuse.
- [x] EngineerPriorityPolicy.h / EngineerPriorityNative.h: RED/GREEN native
  positive-candidate observer and bounded placement metadata. Use an ordinary
  callsite-gated helper detour, not a midfunction register/assembly shim.
- [x] Prove and implement surplus return through the original builder election
  rank and bounded native transfer. Fail closed on ambiguous ownership/storage;
  never substitute broad FILL_MACHINE resource behavior without a quantity gate.
- [x] Integrate lifecycle/input/settings in AIFixed.cpp and personal-safe INI;
  three locales, description of exact limits in README and test documentation.
- [x] Run host/runtime/native admission tests, existing regressions, VC10 DLL
  build/import checks. Independent code/performance/lifetime review; fix findings.
- [x] Only install a reviewable safe result; report what is installed versus
  still unverified. No game launch, new player runs, profiling or publication.

## Review focus

1. Speculative finder calls must never consume real reservations.
2. Direct/urgent/higher normal jobs and foreign packages win without reassertion.
3. Ground stacks and storage quantities differ; credits cannot exceed observed
   pickups or remove preexisting same-type items after merging/consumption.
4. World/actor/source reuse, missing storage and partial/failed transfer cannot
   leak claims, lose items or duplicate inventory.
5. Bounded metadata work, short locks, no callbacks/native calls under locks,
   no extra native scans per frame or hidden background graphics/AI work.

## Known limits and resolved native boundaries

- Chronology before observation cannot be reconstructed from handles/pointers.
  Across-load order needs a proved persisted state contract before claiming it.
- Native haul-to-storage takes whole resource stacks. Our tagged return plan
  uses the verified one-unit transfer gate and original builder election rank;
  unsupported variants/storage retain cargo. Owned credits reset on load.
- The native strength/weight formula is authenticated and batches are bounded;
  unknown/invalid weights retain the native two-item batch. This is not a claim
  of globally optimal packing or path choice.

## Final offline verification, 2026-11-18

`tests/test_engineer_all.py`: all 27 suites PASS on unchanged production hashes.
Includes real policy/runtime host fixtures Od/O2, semantic negative controls,
Steam native windows/vtables, SDK compile, all existing AI Fixed regressions,
three compiled locales and the feature/master OFF rollback path.

Fresh VC10 /GL + LTCG DLL: 310784 bytes, SHA256
7cbcedbaebd9ef555e77eeac5e287982ea9d963e67c13225a2344bd1cd0709ff.
152 KenshiLib imports compatible with installed 0.5.0, x64/startPlugin checked.
Independent final review: no P1/P2 findings remain in the reviewed candidate.
No gameplay, FPS, long-session memory or GOG executable test was performed.
No publication or release package is implied by these results.

Local test candidate installed with CoordinateEngineers=1 appended to the
personal INI. All previous INI bytes/values preserved; old DLL/catalogs/defaults
and INI backed up outside the game mod directory. Installation receipt:
D:/DEV/Kenshi/build/AIFixed/engineer-install-20261118/receipt.json.
Published AI Fixed remains release 3; no release number/package/public metadata
was changed. The installed development candidate has not been run in game.

# Active work and next requested tasks

Updated 2026-11-05. This is a private working queue, not a release announcement.
Latest user instruction, 05 Nov: user is back and running controlled KEO tests.
Do not resume DF or the satellite map without an explicit request.
Avoid taking input from neighboring Minecraft autotests. Window-control tools
currently fail before initialization (kernel assets: OS error 3), including
after one kernel reset; no game launch or UI input has been performed in this
pathfinding audit. Code/build verification can continue independently.
Always check the actual process immediately before replacing a loaded DLL.

1. **Global satellite map — PAUSED BY USER, 2026-11-04.**
   Final private installed build bdb15ea; preserve atlas/settings/source. Final
   actual log: FinePresent about0.02ms percall, LoadedCapture about15ms each1s.
   User still confirms FPS drops and map-icon flicker. No further map changes,
   rendering, restart requests or speculative optimization until user resumes.
   Evidence: game/_codex_work/20261104-satellite-pause/final_measurement.json and
   PAUSED.md. Real1024px parent rendering remains unimplemented; AcceptedCrop
   is only a tested prerequisite. No public release/personal changelog entry.

2. **KEO — ACTIVE PRIMARY TASK, refined by user2026-11-05.**
   LATEST05Nov07:41: permanent fix7609b76 INSTALLED, PROD SHA
   ea56647bb504a5287ee2589b815d9b1ac1cef6e760d6921048420908ede86751.
   All current>=2classes preserve native readiness; hiddenINIkeys cannot opt
   back into false-ready; workers/preload/cache/priorities unchanged. Actual
   hook6144cases eachOd/O2 +OLDpolicyRED +legacydiagnosticfixture passed;
   independentreviewCLEAR, DEV/PROD351/imports180SDK29MyGUI1Ogre passed.
   Original pre-control INI restored EXACT2e3480c8; temporarycontrolinactive.
   Other3personalconfigs unchanged. BackupDDEV/backup/keo-native-ready-20261105-074130.
   Private ZIP KEO-native-readiness-test-20261105.zip793227B14filesCRC,
   SHA4ae278c0fb2662f928e34f4c54af5c9aa5103f4a544ffff734791f1ee3adcdf7.
   Usernowrepeatingfreshimport/outbackforEXACTnewDLLacceptance (notyetconfirmed).
   Both capturedbaseline/control logs preserved. LoK HOLD lifted only AFTER
   controlledtraces saved: exactly3verifiedauthorfiles installedviaPWsh at07:42;
   RU/NCE/Anim/Robot/modlist unchanged. FirstlegacyPowerShellattempt failed at
   Get-FileHash beforeLoKwrites; correctedPWsh installPASS. No publicrelease.

   USER ACCEPTANCE05Nov: single-variable fresh-import control with native
   readiness succeeded: NPC in city after unload/return. Logs frozen at
   KEO-native-ready-20844.log / RE_Kenshi-native-ready-20844.log, bypass=0,
   overrides=off. Runtime Шпилька route quality NOT compared yet (user explicitly
   answered 'Пока не проверял'). Permanent fix being prepared: retire false-ready
   contract for ALL current>=2classes; original answer and backgroundworkers/
   preloading/cache/priorities preserved. Do not use outdoor-only shortcut.
   GOG tester now reports latest supplied GOG test has no errors/crashes.
   Record acceptance of supplied isolated GOG test, not yet new city fix or all
   production jobs; tester DLL SHA was not independently returned.

   CONTROL ACTIVE05Nov07:25: after confirmed diagnostic repro PID26768, changed
   ONLY KEO.ini readinessOverrides=false, same D2EC DLL/mod list/LoK. Exact
   original INI backup game/_codex_work/20261105-squin-second-load/
   KEO.ini.before-native-ready-control; originalSHA2e3480c8, controlSHA8fe46f1d.
   Restore with native_ready_control.py restore ONLY while closed and current
   hash still matches; do not blindly overwrite later user settings. User now
   repeating freshIMPORT->Squin->Hubunload->Squin; may pause at Шпилька detour.
   Actual ON trace preserved: 1994bypasses=557rows+1437coalesced,0drops. First
   Squin18,35outdoorADD49.554 beforebypass49.572; return outdoorRET101.741,
   Mainstate4OUTDOOR_MISSINGbypass120.123,transitionend120.136,ADD120.928.
   This proves early-ready behavior, not yet exactNPCplacementcausality.
   Snapshot557actors;46home-town actors nearSEcellcorner. TownGuard refusals
   all20,31range0; customunload/save0, so no directprivateunloadpath observed.

   LATEST05Nov07:15: diagnostics-only candidate3c54bd6 INSTALLED while Kenshi*
   freshly absent. DLLD2EC3531, all four personal config files byte-preserved.
   Private ZIP KEO-reload-diagnostic-20261105.zip796379B15filesCRC,
   SHA009745173bdea9b982d814e62ffa12612e20b23290bb57652f6a25d71895a7e2.
   Source readiness/town/handoff behavior unchanged: new bounded PROD Debug
   readiness nativefalse->KEOtrue event queue; town refusal arguments/cell;
   private finalize/handoff repeated-load events. Existing unload(save) retained.
   Readiness1536paritycases eachOd/O2, concurrent8000writes and removedrecordRED;
   lifecycle111eachOd/O2+OLDRED; independentreviewsCLEAR; DEV/PROD351/importsPASS.
   User asked to do one importLEO4->Squin->Hubuntilunload->Squin, pause atcrowd
   and leaveopen. CopyONlogs/readactors BEFOREanyOFFcontrol. No spawn fixclaim.
   BaselineON/OFF logs backed under _codex_work/20261105-squin-second-load.
   LoK new upstream prepared in parallel, ACTIVEFILESHELDUNCHANGED untilthis
   KEOtracecopied. RU/NCE/animation targets current; avoid confounding repeat.

   LATEST user repro05Nov: independent MAIN-MENU IMPORT of LEO4 changes prior
   conclusion. KEO ON: first Squin approach residents inside; return to Hub until
   Squin unloads, then return Squin -> residents outside. KEO OFF with same fresh
   import/repeat cycle -> residents inside. Therefore running KEO is implicated
   in unload/reload scenario; older comparison on already-saved state did not
   exclude KEO. Building diagnostics only, no speculative spawn fix yet.
   Preserve ON mods/KEO/KEO.log before OFF control overwrites RE_Kenshi log.
   Copies saved _codex_work/20261105-squin-second-load/*before-diag.log.
   Other new request: compare Steel navmesh regeneration with existing KEO
   wallSpliceFix/spliceSeamFull (already enabled by default). Steel schedules a
   zone rebuild after activation/neighbor readiness and calls furniture repair;
   this is not simply a construction-completion event. Audit missing safe
   coverage separately AFTER spawn diagnostic to avoid changing test behavior.

   LATEST05Nov06:29: private path candidate3e5a11d installed, DLLF8074F90.
   Two concrete source defects fixed: selected actors beyond the first30 no
   longer retain previous formation/planner ownership after a new MOVE; a
   cancellation/replacement during native waypoint snap preserves native output
   instead of publishing stale steering. Cohort and cache caps unchanged.
   345+186 production-extracted controls perOd/O2; old-behavior negative controls
   fail as intended, independent review CLEAR, DEV/PROD351-source builds and
   180SDK/29MyGUI/1Ogre imports verified. No measured gameplay/FPS/leak claim.
   Installed only DLL after fresh Kenshi*process count0; all four personal
   configuration files byte-preserved. Backup DDEV/backup/keo-path-20261105-062901.
   Private archive workshop/testing/KEO/KEO-path-test-20261105.zip:791284B,
   SHA9a32dbd9955297c8e24d9b34875b29fb62837c2c69de0fa66abb1c19d8b324f7.
   Original route-accepted df07c25 package remains available separately.
   Existing NPC log shows STATE-FULL caps, native once/unboosted; repeated faces
   do not prove one retrying NPC. Need existing DEV NpcCapWho ownership capture
   before any suppression or geometry rewrite. Do not call43.3Havok units43m.
   Window tools failed before initialization even after reset, so no game launch
   or input occurred. User asleep: do not ask for scene preparation overnight.
   GOG tester reply still pending; separate GOG archive unchanged. No publication,
   public version bump, or ALL_CHANGELOG entry. Map/DF remain paused.

   LATEST05Nov05:48: installed private DLLFB42DE66, sourcedf07c25.
   User confirms fixed-vtable build no longer crashes when adding a waypoint,
   but only one row appeared. Actual read-only PID13116 proved one292 row with
   valid subject/serial/coordinates and TaskData.fixedTarget=false. Native
   OrdersReceiver.addJob507D74 rejects same-type rows without comparing targets
   when that flag is false. Changed only factory fixedTarget=true for all three
   jobs; distinct targets permitted, identical targets still rejected.
   206actual factory controls + false-only RED and DEV/PROD passed. Installed
   while fresh process absent; all four personal configuration files preserved.
   User CONFIRMED corrected two-point loop works, then saved a separate
   KEO-route-test-20261105 slot and loaded without import: both points restored
   and route continues. Steam route creation/repetition/save-load accepted.
   Exact installed DLL verified; full production transfer/GOG/FPS/leak acceptance
   remains separate, not implied by this route test.
   Previous actual crash: truncated derived vtable, fixedb59ab75/5E01E06E.
   Early metadata/late template registration now confirmed in actual Steam.
   Counter hold-RMB command exists/adds; no physical delivery acceptance yet.
   Production jobs remain optional/defaultOFF; user doesn't personally need them.
   Previous04:39: three Steel jobs IMPLEMENTED and INSTALLED LOCALLY in a
   private candidate. Source ac2c92e7 (after770f8d2 integration and2bf6df9 SDK fix),
   PROD abf949c03c829dd23a737e41286293cc6cfb3009e9bb168caa863d4c9789db20.
   MCM has independent live/default-OFF Gather Output291, Stock Counter293
   (physical own-counter deposit, NO monetary trade) and cyclic Waypoints292.
   Native rows survive OFF; route coordinates restore from native saved hand
   payload; no242 migration. Native factory/common initialization is retained.
   Bounded whole-stack transfer restores the same remainder to the reserved
   source footprint, measures charges/quantity conservation and refuses unknown
   inventory handlers/provenance. Foreign TaskData ids are passed through.
   DEV/PROD351-source builds/180 SDK0.5 imports passed; focused actual-code,
   input/caption/lifecycle/ownership/transfer and exact Steam/GOG PE controls
   passed; independent scoped reviews completed. In-game acceptance pending.
   Private jobs ZIP: workshop/testing/KEO/KEO-jobs-test-20261105.zip,786224B,
   SHA0087cc0e67a470b7e5cdf69cefafb64aa7a832cfcea28b771eeb9e38e64fa3b7.
   All four personal INI/REjson/mod order files byte-preserved; backup
   D:/DEV/Kenshi/backup/keo-jobs-20261105-043907. User asked to launch/open MCM;
   no game process was present at installation. Do not reuse historical PIDs.

   LATEST GOG retry: tester got GetRealAddress assert KEO.dll+F1880 and weight
   contract refusal. Exact delivered DLL resolves that address to a LOCAL
   virtual getAge thunk; getRace was likewise wrong. Source2bf6df9 uses exported
   _NV_getAge/_NV_getRace ONLY for address checks, preserving virtual gameplay
   calls and every guard. Actual old-code RED/new-code GREEN verified.
   SEPARATE fix-only GOG ZIP rebuilt from committed2bf6df9 export WITHOUT the
   three jobs: workshop/testing/KEO/KEO-GOG-test-20261105.zip,767654B,
   SHAaf4fb6d80697e18541ac384219329330989dafb764353fd37a9e738f6886c191;
   DLL0e86f8a6b779543a8ca7fdd055ebf307829e762c4168e635aa93c0be59534c56,
   150 SDK imports,12filesCRC. Real GOG startup still requires tester retry.
   Earlier au-mapping fix84056bcb remains; this is a distinct feature-specific
   SDK usage error, not a reason to disable the startup guard.
   No public release/version increment/personal published changelog entry.
   Gameplay priorities: (1) pathfinding: short-route detours, circling/stuck
   movement, squad routing and avoiding water; (2) excess squads: distinguish
   genuinely created platoons from existing platoons loaded earlier; (3) mouse
   input lag is LOW PRIORITY, a single-player unconfirmed report, absent locally.
   Investigate input only with a concrete lead or reproducible evidence.

   The outside-Squin crowd also reproduced with ZERO KEO/KEOCompat modules in
   actual PID30780. This excludes running KEO as necessary for that observation,
   but not already displaced saved positions. By explicit user request both
   spawn/readiness experiments5772425 andfb8c46a were rolled back. Current source
   022bb70 has native src exactly5f68039, retaining MCM, compactRU/EN/ZH labels,
   navmesh/priority/pause/TLS/jobcancel fixes. Installed DLL32c40117...653423.
   Original personal INI and mod order were restored; no temporary test override
   remains. Do not reintroduce these spawn experiments as a proven fix.

   Latest actual INI check05Nov: keoRouting=true/plannerMode=on, enabled by the
   user. The user says routing currently appears to work, but broad gameplay
   testing remains required; do not equate this with proving every route fixed.
   Preserve the user's settings and measure the active loaded DLL each session.

   GOG NATIVE CANDIDATE READY05Nov: local source e6082d6, full production DLL
   eaa75821e941f85fe95b4e3a7ce7fa3fde17c564d03a508a48132b85303b2859.
   Private tester ZIP workshop/testing/KEO/KEO-GOG-test-20261105.zip, CRC/exact11
   files/144SDK+29MyGUI imports verified. Real GOG gameplay remains UNVERIFIED;
   package is not installed or published. Patch backup is under
   tools/maintenance/keo/native-gog-e6082d6264b6.patch in our own repository.

   GOG provenance: user supplied game/DropMeFiles_VfFSw.zip
   containing two EXEs. The smaller RE image36708352B matches GOG1.0.65 MD5
   213349bf76a0a758067f9ed0aef2ab01. Sources were not executed. Evidence at
   game/_codex_work/20261105-keo-gog-native. Build a verified native profile,
   including raw-base accesses, inline patches and anonymous return/byte tables;
   mapped exported entries alone are insufficient. Do not offer startup-only
   stubs as a working GOG port. Keep other-module Ogre/PhysX checks separate.

   Preserve native multithreading/player intent. MCM/RU/EN/ZH and detailed-log-last
   rules apply. Temporary maintenance fork: credit the original author and
   withdraw our version if original development resumes. Never push to the
   author's origin. Back up local KEO commits as patches in our own repository.

   NEW05Nov user request: compare actual KenshiQOL sources with KEO and port
   useful missing features carefully, including dynamic carried-body weight.
   Source: workshop/content/233860/KenshiQOL/src, Workshop3772744830, author
   Steel confirmed by its original page; bundled GPLv3 retained. Read-only audit:
   game/_codex_work/20261105-kenshi-qol-audit/report.md and evidence.json.
   Identified overlaps: global backpack routing vs KEO wearer choice; QOL zone
   auto-rebuild triggered by KEO preload; nearest-town preview overwrite;
   shared getDropped/order hooks. Do not copy the combined navmesh switch.
   FIRST TRANSFER INSTALLED05Nov03:07MSK: source f568b971, PROD DLL
   d53062b8bc2b0dee5107ad5cc4174de10b73a7267507411042fb57e20bb58f37,
   1489920B; opt-in carriedWeight default OFF and MCM restart marker. Personal
   KEO.ini/KEOCompat.ini/REjson/mods.cfg byte-preserved, backup
   D:/DEV/Kenshi/backup/keo-weight-20261105. Root repeated125/411/13 Od/O2 and31
   native contract controls; independent review CLEAR; both native builds and
   config/layout/MCM regressions PASS. Private test ZIP
   workshop/testing/KEO/KEO-weight-test-20261105.zip, CRC12files verified.
   Real weight gameplay/FPS/memory/save-load acceptance is still pending.
   The three jobs are now installed as described above; gate/drop changes are
   NOT ported. Gate native registration gap
   depends on data flags; do not claim universal missing registration.
   Upstream post-drop copy treats native query -1 as success; never copy that.
   Implementation: opt-in dynamic body/inventory weight, startup-only,
   bounded allocation-free traversal and thread-local recursion protection;
   disable this feature if the original KenshiQOL DLL is loaded. Preserve native
   inventory weight semantics, robot/hunger distinction and save data.
   Next isolated candidates: gate registration and post-drop building handle.
   Implemented design constraints for the new jobs: separate capability and
   save-lifecycle admission; Gather Output, counter stocking and queued movement.
   Do not silently migrate all native task242 rows or modify saves with OFF.
   User clarified05Nov: expose all three jobs as separate MCM switches with
   meaningful descriptions. Actual upstream job291 Gather Output empties machine
   outputs even without a matching storage demand; job293 Stock Shop Counter
   moves the output types of that actor's Gather jobs (or native ditchable
   resources when none exist) into a counter, without money changing hands;
   job292 Waypoint Route loops through its Jobs-panel waypoint rows. Preserve
   saved rows when OFF, suspend only our own task execution/assignment, and keep
   dormant deserialization safe. Never turn unrelated native jobs into ours.
   For stocking, verify ownership and capacity, bounded transfers and loss-free
   rollback; do not inherit upstream's unguarded arbitrary shop destination.

3. **Dynamic Factions — PAUSED BY USER.** Preserve all work and plans. No
   new implementation, game testing or publication until explicitly resumed.

4. **Save screenshot previews — IMPLEMENTED/USER ACCEPTED2026-11-01.** Capture the
   game window/frame when a save succeeds; display its preview below the save
   list when selecting that slot, both from the main menu and during play.
   User reference screenshots16:56:51/16:57:14 show placement. Latest user
   instruction fixes the module: **UI Tweaks**, implemented alongside satellite.
   Detailed task: docs/superpowers/specs/2026-11-01-save-previews-backlog.md.
   The screenshot includes HUD, minimap and character panels; exclude only
   save/load/pause menus and their backdrop, not the entire interface. Use the
   actual native game target, never a desktop/menu screenshot. The preview spans
   the dialog width; preserve image aspect ratio and cap height to available
   space. Hide the entire panel before selection or when no image exists.
   Latest user refinement: show existing-slot previews in SAVE as well as LOAD;
   raise the dialog once on opening, never jump on selection or crop the image.
   New snapshots use filtered1600x900 lossless PNG; legacy640 JPEG stays readable.
   MCM height100..160%, default120%, applies on next opening; list must grow too.
   Auto/quicksave preserve the visible HUD too. Existing saves without image
   remain usable; only confirmed successful saves receive a new preview.
   Installed b6a0589 candidate328192B; actual accepted test sidecar1600x900 PNG
   matches its native save revision. Full automated/native-geometry review passes;
   long-run performance measurement remains separate. Prepare the next public
   release once, relative to the recorded published baseline12, not repeated bumps.
5. **Container Highlight maintenance — installed2026-11-01.** Confirmed
   stale-model restoration retry fault addressed in fork commit9101ad8. Earlier
   fixes covered other paths; the player's exact freeze remains unverified.
   48 extracted production checks and independent review pass. Installed matching
   ba4ce0f candidate; PID13024 startup confirmed. Runtime travel/interior/Alt/save-load
   acceptance remains pending; do not claim the player's exact incident cured.
6. **Organize the Trader unsupported locale — audit closed2026-11-01.** English
   fallback passes for current and v7 loaders with packaged catalogs, including
   German variants. No production defect/new language added. User accepts this
   result; reopen only with concrete symptoms or logs.
7. **Item Markers known-location maps — IMPLEMENTED/USER ACCEPTED2026-11-03.**
   Same lower-left eye as learned blueprints, only when all original listed
   locations are discovered. Independent MapsKnown setting, three languages,
   bounded shared visible400ms snapshot and fail-closed unknown/refusal cases.
   Root and independent tests/native build pass; reviewed DLL and threeMO are
   installed and the user confirmed the known/unknown map check. Version7 ZIP,
   bilingual guide and notes were published as version7 on2026-11-04 after the
   user uploaded Nexus. Steam EN/RU and the personal release journal are updated.
   Do not repackage this unchanged feature as a new release.

## Maintenance completed in this continuation

- **LoK after author's FPS update: NCE checked2026-11-01.** Installed upstream
  `.mod` differs from the earlier file, but every ARMOUR record is unchanged.
  Regenerated NCE candidate is byte-identical to installed THE_NCE_Patch; no
  replacement is necessary. Evidence in
  `translations/LegendaryOfKenshiRU/nce-verification-20261101.json`.

## Completion boundary

A diagnostic render or screenshot is not completion of the satellite feature.
Completion requires an actual optional layer in the global map, user controls,
full requested geometry/lighting coverage and measured stability/performance.


## Deferred release batch: detailed localization logs (user, 2026-11-01)

COMPLETE2026-11-02: user uploaded all37 Nexus packages; all37 corresponding Steam
versions/content/ENRU descriptions/previews and single bilingual notes published.
Dark UI Advanced/Formations received description-only changes. See
docs/releases/2026-11-02-steam-publication.md and synchronized published ledger.
OTI/XP_Overhaul remain local; DF/global map remain private. Do not prepare or
publish this same batch again or increment versions merely for local fixes.


2026-11-02 user follow-up: optional global-map biome zones, diplomatic zones,
and road/path overlay layers, alongside the previously planned exploration fog.
Separate MCM toggles; native source evidence and no atlas recapture. See
GlobalSatelliteMap/PLAN.md. Future work after the active core coverage fixes.


## New MCM task — reset confirmation (user2026-11-05)

Implement centrally in Mod Config Menu for every plugin's common Reset button.
Clicking Reset opens one modal with a red warning that this plugin's current
settings will be lost. No setter, INI write, apply or reset before explicit
confirmation. Cancel/default focus, Escape and close dismiss safely. Changing
plugin/page, closing/rebuilding options or repeated clicks cannot reset another
plugin or retain a dangling UI/plugin reference. RU/EN/ZH, inherited vanilla/Dark
skins, bounded lifecycle and meaningful actual reset-flow tests. IMPLEMENTED and
installed locally 05Nov01:30MSK; MCM13 ZIP/ENRU descriptions prepared. Independent
review CLEAR; native VC10 GL/LTCG and actual reset45/modal51 checks Od/O2 pass.
User confirmed the in-game cancellation/appearance check: works. Private local installation
receipt: docs/releases/2026-11-05-mcm-reset-confirmation.json.
Published baseline is MCM12 (confirmed ledger); one next public package is13,
not one increment per intermediate fix. No Nexus/Steam publication authorized
for this new package yet; no personal ALL_CHANGELOG entry until published.

# Private WorldBuild caller review — 2026-11-03

Verdict: CLEAR for the reviewed caller source; no open P1/P2 finding. Final native candidate identity/build now verified below; independent disk-factory review remains a separate integration gate. This review did not install, execute native APIs, open the game, or change production files.

Reviewed `CaptureProbe.cpp` diff against 9428d14 plus new `WorldBuildMain.inl` and `test_world_build_main.py`. Exact source hashes are in source-snapshot.json. The extracted Update/callback blocks in branches.cpp are byte-for-byte equal as text to their production blocks, independently checked after testing. The current early Update scope comparison cancels a same-address load-generation change before PrepareWorldBuildJob/Queue; compositor Tick independently receives a freshly reconstructed scope.

## Evidence

- Fresh actual-inl caller tests: 6,219 checks each /Od and /O2. Invalid case rejected before owner/native snapshot, default case0 region27/cell10, valid K4 jobs, all1,024 region-major source cells and4,096 independent window placements, exactly64 regional material snapshots, full-width world/load freshness, and graceful terminal count checks. Native material values are explicit host stand-ins; this does not execute GameData or Ogre.
- Independent actual extracted Update/callback tests:13 each /Od and /O2. Hidden map/widget, invalid context, prior stop, polled cancellation before queue, inside/reentry, quarantined owner, current load generation in Update and callback, competing diagnostic refusal, and typed/unknown callback exceptions cancelling the consumer before returning.
- Three compiled semantic mutants fail their intended assertions: remove early load-scope refusal; forward captured scope instead of current scope at compositor Tick; omit consumer cancellation from the unknown-exception callback catch.
- Host PE imports inspected before every executable run; no Ogre/Kenshi DLL imports. Native probe DLL is never loaded by this review.

Reproduce with bundled Python: `run_caller.py`, then `run_branches.py` in this report directory. The former redirects only test output to this private directory; it includes production WorldBuildMain.inl. The latter compiles the preserved exact extracted blocks with test-owned native stand-ins. Mutant sources, binaries and compile/run/import logs are retained.

## Ownership, admission and scope

WorldBuild is startup opt-in/defaultOFF; Request0/current startup token are latched without replay, independent from other diagnostic modes. Startup/current viewerOFF and absence of its textures are required. Main Update prepares native metadata and queues only while ready; D3D/progress runs from the existing Main compositor callback. Main does not create the output directory or read source image blobs: Config owns fixed paths and the reviewed worker/factory performs those operations. ParametersRegion(false) copies900+60 floats once per region, validates native role/SID/selector identity, and neither mutates native geography nor allocates native terrain geometry.

Update invalidation and both exception catches signal parent Cancel. Parent owns consumer-first stop/join and producer lifetime; caller does not directly Pump/Cancel/Release the producer behind its borrowed writer. Unknown waits retain that bounded whole owner; cleanup only detaches diagnostic listener/resources after parent terminal/quarantine reporting. Existing listener forwarding remains outside the new branch; current workspace/renderer/world and MapWidgetRef visibility checks precede admitted render work.

Case0 produces4windows; Case1 traverses64regions×16cells×4views. All output remains a new private incomplete GST2 directory: no existing atlas overwrite, mip/ready/index or publication claim. Terminal logging additionally requires expected whole-cohort and window counts.

## Performance and limits

No per-frame world/GameData traversal or image/blob IO is introduced. Regional metadata validation occurs once per16cells and planner/job preparation once percohort. CPU copies/job/path scratch are charged with a conservative512KiB external allowance in addition to the generic parent/provider/writer ledgers. Shaders are static pinned arrays; per-view resources/worker/unknown-wait retention are governed by previously reviewed bounded owners.

The inherited FixedContext guard checks viewer INI and enumerates the texture registry (hard refusal beyond32,768 entries) during active Update and compositor callback. This is a bounded resource-manager scan and may run twice perframe; its real hardware cost and long-run performance are not proved by host tests. The new path does not invoke legacy atlas rendering or native geometry allocation.

No claim is made for complete all-world native material acceptance, actual-game output quality, FPS, leak absence or atomic all-world GameData mutation snapshots. The native API/profile and complete caller DLL require the parent’s final build/import/profile evidence and actual guarded-game acceptance. Existing partial files may remain as diagnostic data after refusal; they are not a published atlas.

## Final native identity amendment

The completed full probe build log is `build/world-build-native-build.log` SHA2562a63b58f9d188500de25ec269a2f2619c27cd9874eb7d148d6dbcb777dd98f5f; its final production source/startup/PE contracts pass. Reviewed CaptureProbe/WorldBuildMain source hashes remained unchanged through that build. Actual candidate DLL is852,992B, SHA256c853e46d00de5307290ac9d5da47ac572dbe2bb0002c1bd2ea1e6f507de7275a, independently hashed without loading it. The parent candidate manifest is copied as native-candidate.json; it also pins final disk header78a93d3bc2f82b181c1e3897f4316379aebd1d1ac175bd3e4d0fd1280605e858. No extra suite was added for this identity amendment. Disk-factory peer review and actual-game acceptance remain separate gates.

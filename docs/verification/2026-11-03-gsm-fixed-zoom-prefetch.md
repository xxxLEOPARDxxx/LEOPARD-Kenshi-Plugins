# Fixed zoom and bounded cache-ahead candidate

Installed locally, not publicly released:GlobalSatelliteMap.dll1,111,552bytes,SHA6b61614096d784bfee4f1cf003b3a4dff094fa8573c65ccb553d6b3b2b90663b. Personal INI unchanged; existing atlas not rerendered. Game was checked closed through Win32_Process before replacement. Await live acceptance.

Fixed zoom uses a deterministic native-range ladder and exact extension factors1,1.25,1.5,2,2.5,3,4,6,8 within the configured cap. The native original applies the selected target and performs center/cursor anchoring; no post-call field snapping. Scoped bounds and depth restore on unwind. Temporary bounds cannot enter ApplyZoomLimit/Tick recursively. Satellite-off/foreign bounds keep native behavior; normalx1 mode is accepted only with validated captured native bounds.

Ordinary cold window streaming can otherwise admit only one result per200ms Draw. Pending ordinary-only file work now permits50ms Draw cadence. Loaded idle and active fine/source work retain200ms; the shade budget remains four windows and file IO stays in the worker.

At max zoom, new edge windows require new native GPU production. Existing file/height CPU workers do not make that render pass freely parallel. A55-window cold view measured roughly40seconds; this is not a CPU-core saturation claim. Two directional adjacent pairs at most can be prepared after visible production with600ms stable demand. Speculation uses the existing bounded worker/Put-ack/drain path, never uploads speculative GPU images or evicts visible images just for speculation. Optional source/profile refusal skips speculation and continues visible publication. Existing192MiB GPU/128disk-slot limits remain. Full visible coverage can still wait for cold generation; map close currently discards the healthy temporary fine cache, so reopen is cold again.

Verification:27 pure fixed-step controls,32 extracted native-wrapper controls,8 actual cadence controls in each/Od and/O2; existing zoom lifecycle28controls+8negative variants pass. Prefetch37 extracted caller/policy controls+3functional mutants,26 actual WIC/WARP/store controls per optimization. Independent mutual review CLEAR. Native linked candidate has required startup/MCM exports and84 verified native imports. Compact records are in tests/fixed-zoom and tests/fine-prefetch; developer machine-only freeze receipt is D:/DEV/Kenshi/research/gsm-fine-prefetch-20261103/verification.json.

This is not complete town geometry, fog of war, road/biome/diplomatic overlays, a GOG acceptance result, or a long-session FPS/leak guarantee.

# World-window session implementation

1. Generate two owned actual WARP512/9m fixtures aligned to the32768 atlas at centers(-16128,-16128) and(-11520,-16128), using unchanged raster/bank and authenticated height samples. No borrowed output writes.
2. Add actual Accepted→session→temporary GST2 tests and an E_NOTIMPL stub; observe runtime RED.
3. Implement exact world-address mapping, one-shot worker-owned scope/capture+atlas-material binding, lazy max16 fresh files/writers, one shared accepted-window adapter,64MiB explicit reservation, duplicate detection, partial/cancel poison and cleanup. Per-window Frame material generations remain independent and freshly authenticated by Accepted.
4. Verify exact real temporary-file readback, independent source-height samples, offsets/flags, refusal/no-IO paths, worker ownership, cancellation and IO faults; compiled mutants must fail meaningful runtime assertions. Freeze source/dependencies/fixtures/logs. No mips, readiness, scheduler or integration.

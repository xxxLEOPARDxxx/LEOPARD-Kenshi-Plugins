# Checked native provider rebinding

New directory only; no frozen controller/provider/helper edits or native execution.

- [x] Pin idle admission, same-scope token replay and fail-closed capture behavior.
- [x] Own one actual controller plus replaceable terminal per-cohort provider.
  Verify that controller's idle boundary, and preserve original raw native
  snapshots behind per-binding stamped public snapshots.
- [x] Route Capture only within owned capture transaction; translate checked
  Refresh/BeginBank back to original snapshots and keep native bank serialization.
- [x] Account fixed snapshots/tokens, bank scratch and old/new provider peak.
- [x] Run real frozen StageAdapter fake-stage controls, repeated actual WARP
  cohorts with one height authentication, and compile-only actual native default.
- [x] Freeze exact evidence and request independent review.

Author evidence complete; independent review is pending. Native default was
compiled/linked only, never executed by the host controls.

Owning the controller avoids pretending a supplied unrelated controller or bool
proves this provider is idle. Caller must still obey the frozen consumer-join
contract before final ReleaseView makes that controller Idle. A capture failure
after admitted replacement invalidates/deletes old and candidate providers; it
never leaves the previous binding ready. Rejected busy admission does not replace
or destroy the active current binding.

# Joined operation errors and transient metadata contention

`run.py` extracts the current production `Queue`, `RequestShutdown`, and
`PollShutdown` bodies without rewriting their decisions. Small native stubs
represent proven terminal, pending, quarantined, exposed-read and GPU-owned
states; queue admission/geometry are assumed valid to isolate this lifecycle
contract. It is not an engine, native lock, source-identity or FPS test.

VC10 x64 `/Od` and `/O2`: 53 controls each. Historical `ERROR_BUSY` after a known
native join preserves the original HRESULT/first error and releases the owner
without quarantine. Unknown native failure retains the owner. Foreign-thread,
consumer-not-joined and exposed-read guards do not call native teardown.

Synchronous capture contention is retryable only at the source's Idle/GPU-zero
boundary; it leaves Error healthy, never submits an unbound job and allows the
next Queue to succeed. A separate transient flag resets on each admitted Main
Queue attempt. Non-Idle/GPU-owned, other errors and Submit-stage BUSY remain
fatal. The two small pre-fix RED logs are retained beside this file.

The live diagnosis used only ReadProcessMemory: PID3580 had controller Error
`0x800700AA`, quarantine1, but native state Failed, quarantine0, work/thread/query
NULL and consumer work/thread NULL. Thus the outer quarantine incorrectly
treated a historical operation HRESULT as an unknown lifetime. Native final
stage0/input8 identifies the metadata-accept boundary; root separately fixes
the exact native lock-contention retry points. This test does not claim a new
live game run has accepted the candidate.

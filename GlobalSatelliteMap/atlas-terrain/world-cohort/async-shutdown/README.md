# Idle worker asynchronous retirement

Production API: `WorldCohort::Controller::RequestShutdown()`, `PollShutdown()`, `Quarantined()`.

Request is Main-only and accepts only Idle with an actual WIdle worker, a live thread, no reserved GPU bytes, no query owner/pending query, and no accepted borrowable view. The caller must already have joined every consumer and drained/closed the producer. Stopping is appended after existing state IDs. It sends Stop and wakes the existing worker; it never waits for completion in that call.

Poll uses a zero-time thread wait. S_FALSE means still pending. Only a signalled thread with WStopped may release Work, handles and the global worker slot. Terminal worker HRESULT is read before Work is deleted. Completed polls preserve the result. Unknown waits, missing cleanup marker or the wrap-safe five-second deadline quarantine and retain the CPU/GPU owners and worker slot. A wake failure after command publication is latched, but Request still returns S_OK so ownership remains on the polling path. Cancel cannot turn Stopping back into Cancelling; ordinary Pump dispatches to Poll while Stopping. The existing blocking Shutdown remains final teardown.

## Verification

Run `run_tests.py` with bundled Python. It compiles actual production header/method bodies with VC10 /Od and /O2; its generated header only makes existing private Controller fields visible to the harness. Host executables import no Kenshi/Ogre DLL. A real WIC/COM worker boots and exits in every ordinary test. No engine, source job, raster or D3D device is substituted as a success.

The shim only supplies controlled wait/tick failures at the actual OS boundary. Coverage includes active/Complete refusal, fake retained GPU and query owners, foreign Main, pending poll, Cancel/Pump during stop, exact cleanup marker, high-bit terminal HRESULT before deletion, 12 successive worker-slot reuses, actual unsolicited worker wait failure, initial/poll WAIT_FAILED, missing WStopped and tick rollover at 4999/5000 ms. Each quarantine injection runs in a separate process; process exit reclaims deliberately retained test owners. This is not production recovery and is not a claim of an indefinite leak/FPS test.

`--red` compiles the corrected harness against the pre-change HEAD header and records the expected missing API. `red-missing-api.log` is compact evidence. Generated binaries are ignored; verification.json hashes the delivered source/test contract and retains compact per-case results. Parent owns broad integration freezes, native build, independent review and game validation.

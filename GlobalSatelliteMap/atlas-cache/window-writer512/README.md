# Bounded512 GST2 base-plane writer

Worker-only `WindowWriter512.h` streams512-square windows into a **new,
incomplete** linearRGB16F GST2 tile. It preserves the existing8192 tile and
48 coarse-plane completion bits. It never opens a path, overwrites an atlas,
publishes readiness, creates mip levels or validates GPU pixels.

The caller supplies its fresh `RandomFile`, one plane at a time (at most2 MiB),
and nonzero64-bit source/material/capture generations. Tile coordinates and
512-aligned pixel coordinates must match. A generation mismatch or any partial
write, flush, callback exception or cancellation permanently invalidates this
writer. Data is flushed before its coarse completion bit, which appears only
after all16 subwindows of that2048 block have completed. Duplicates do no IO.

Calls are serialized on the worker owning `Begin`; `Cancel` is the only
cross-thread method. Join that worker before status access, destruction or
release of the file. C++ exceptions are caught; this is not a Windows SEH guard.
There is no resume API. Complete base planes still need the existing mip/hash/
index verification and atomic atlas publication before they become usable.

Run bundled Python `run_tests.py` with VC10 x64 and SDK7.1 installed. Tests
exercise production code,768 plane windows, all48 ledger bits,64-bit generations,
thread ownership and joined cancellation, IO/flush failures and callback unwind.
An independent non-square actual-file check temporarily creates a new1,431,307,264
byte logical file under ignored `_generated`, then closes and deletes only that
unique test file. The test does not claim the file is sparse. Fourteen compiled
mutants must fail runtime checks. No installed atlas or game process is touched.

# Debug CPU stage timings

`GsmCpuTiming.h` stores fixed Main/session aggregates for Lighting, Water,
LocalSnapshot, ModelSnapshot, FineSourceTick, FineModelPrepare, FineLighting,
FinePresent and FineCoverage.
ModelSnapshot is further divided into TownCatalog, TownSelection, LoadedCapture
and DistantCapture to distinguish native model inspection from other work.

```cpp
GsmCpuTiming::Stats cpuTiming;
{
    GsmCpuTiming::Scope timer(debug,cpuTiming,GsmCpuTiming::Lighting);
    Lighting(now);
}
// Once per caller-owned5second report interval, only with debug enabled:
Diagnostic(GsmCpuTiming::FormatReport(cpuTiming));
cpuTiming.Reset();
```

`Reset()` clears aggregates but retains session QPF. `ResetSession()` also
withdraws the cached frequency attempt. The first enabled valid scope queries
QPF once; scopes query QPC only at entry and exit. Disabled scopes do neither,
change no aggregate, and allocate nothing. Bad frequency/counter/backwards
samples are dropped. Tick/call aggregates saturate instead of wrapping.

Scope times are inclusive. Nested FineModelPrepare/FineLighting values must not
be added to enclosing FineSourceTick as exclusive CPU time. These measure CPU
elapsed submission/readback call time, including driver waits; they do not
measure asynchronous GPU completion or prove GPU load. Formatting alone may
allocate; it runs only for the aggregated report. No file/UI/D3D/flush calls are
made by timing scopes. Root owns the serialized stats lifetime, debug gate,
report cadence and fixed stats storage accounting.

Run focused controls:

```powershell
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' GlobalSatelliteMap/tests/cpu-timing/run_tests.py
```

Nineteen controls cover real header behavior with a deterministic fake clock,
disabled query/allocation counts, cached/missing frequency, nested durations,
interval/session reset, counter errors/rollover, saturation, formatting and a
real Windows QPC smoke check. No game assets, graphics device or plugin build.
Initial absent-profiler behavior was observed RED before implementation.
Generated executable/snippets/logs remain in ignored `_generated/`.

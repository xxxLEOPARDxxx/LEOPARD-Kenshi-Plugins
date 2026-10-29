# Temporary MyGUI counters

Developer diagnostic only; not part of the public UITweaks package. Requires the
verified local VC10 MyGUI ABI. Build with the repository build.ps1 and load
explicitly alongside UITweaks only while investigating a prepared game scene.
Remove the diagnostic from RE_Kenshi.json and delete its DLL/INI before release.
The release preparation command rejects an active diagnostic loader.

`LayerProbeLite.ini`, section `[Probe]`: Enabled=1, TraceMiddle=0, Phase=hud-counts.
Configuration is polled once per second. Counters are emitted every ten seconds.
Optional Middle traces sample every64 dirty events, capped at64 stacks/second.
Names and caller tables have fixed capacities; collection fails closed when a
required hook or ABI check fails. Layer context is scoped and names are copied;
no deferred dereference of layer pointers is needed.

Visits count RenderItem invocations, including empty ones. Textured/nonzero
batches are not exact GPU submissions. Attempts count dirty/forced rebuild
attempts, including failed buffer locks. Entries count attempted DrawItem slots,
not GPU draw calls. Frames count LayerManager calls, not guaranteed presents.
Trace overhead varies with dirty-event frequency; use an unloaded diagnostic
for final FPS measurements. This tool does not prove the absence of long leaks.

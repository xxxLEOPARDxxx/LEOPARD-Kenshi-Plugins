# Value-only world Town catalog

2026-11-02. New GsmTownValues.h and focused tests only. No main edit, journal/file write, process/game/native getter call, UI action, plugin build, installation or commit was performed here.

```cpp
GsmTownValues::Result values;
HRESULT hr=GsmTownValues::Read(enabled,verifiedNativeSource,shou,
    GetModuleHandleA(NULL),generation,&GSM::generation,values);
```

The helper itself calls no engine/SDK function or module lookup. Root supplies the already verified actual engine module and live SharedKing lifetime. It reuses GsmTownDiagnostic's bounded SharedKing+8→TownList, uint32 count+58/array+60, exact primary Town vtable and code/thunk guards. Two additional native code24 windows establish ID/position fields: D6360's filename path at D63A0 adds Town+58 and reads uint32+C, therefore Town+64; native926425 copies stored Town+48/+4C/+50. The exact504b EXE and native `distant_`/`.mesh` literals are independently audited by the tests. Neither filename function nor lazy Town::getPosition is executed.

Result fields are generation,total,count,nullEntries,otherTypes and towns[4096]. Each24-byte TownValue contains unsigned id,float storedXYZ[3],int proxyState,bool hasProxy. Exact primary Town entries are copied in list order. Null list entries and other/nonprimary vtables are counted and skipped. ID0 and full unsigned IDs are retained without deduplication. Nonfinite coordinates, malformed/inaccessible data or wrong proxy owner refuse the whole result. hasProxy means an already existing owner-matching DistantTown object; it does not require a loaded Entity, does not read its Entity field, and is not a loaded-model guarantee. Missing proxy is false/state-1; all present native state bits are preserved, including0/1/2/3/4 and unknown signed values.

Stored coordinates are copied bitwise, including negative zero, zero-height sentinels and nonzero stored Y. No terrain sampling, height refresh, name access, visibility/cache mutation, zone activation, filename generation, asset loading or guessed coordinate transform is introduced. The result is source metadata for a later asset join, not proof of native cache signature freshness, complete geometry availability, absolute-to-scene registration or a valid future placement policy for unresolved Y.

Before publishing, the reader rechecks every source array member, every nonnull vtable (including skipped types), every accepted ID/XYZ bit pattern, proxy link/owner/state, list owner/count/array and live generation. Native reads are explicitly volatile under O2/GL. Header/generation are checked before and after the identity pass; generation is checked again after copying output. Any refusal/fault/change clears the result to empty, including the final post-copy check. This is still a synchronous serialized-lifetime contract, not concurrent destruction safety or universal ABA detection.

TownValue is24 bytes; Result is98,324 bytes, below128KiB. Transient identity storage is98,304 bytes and the private pending result98,324 bytes, about192KiB stack plus the caller's Result and small scalar frames. There is no heap allocation, COM reference, persistent native pointer, cache or worker. Both scans have a4096-entry hard bound. The caller owns Debug-only once-per-generation scheduling and local journal logging; calling this every frame is unnecessary. Static bounds are not measured frame-time or long-session memory guarantees.

TDD: an inert initial helper failed the actual owned catalog-publication assertion. Fresh `tests/run_town_values_tests.ps1`:73 actual production-header checks PASS, exact504b filename/ID/XYZ source audit PASS, seven compiled functional mutants rejected and full typed SharedKing SDK VC10/O2/GL compilation PASS. Tests include4096/4097, null/other types without reading Town-only fields, inaccessible source/proxy pages, no Entity dereference, all proxy states, exact id/XYZ values, no lazy-height writes, code pin refusals, and13 deterministic mid-read changes to a later town/header/member/generation with no partial result. Mutants enforce the ID offset, field profile, finite coordinates, full value/array rereads, generation and proxy ownership. Test-owned pages and exception observers are released; no game module is invoked.

Root owns first real420-town acquisition, the journal, joining against actual282 cache assets and any subsequent private LOD loader. This scalar reader grants no permission to activate towns or simulate unloaded zones.

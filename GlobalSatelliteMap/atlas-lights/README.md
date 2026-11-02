# Native registered local-light snapshot

Value-only, C++03, render-thread component. Not wired into the viewer. No renderer, HLSL, MCM, native writes, texture/material loading, scene activation, world traversal, worker, installation or game call is added by these files/tests. Root owns integration and any first live diagnostic. The native reference contract is `D:/DEV/Kenshi/research/gsm-local-light-snapshot-20261102/contract.md`.

## Caller contract

```cpp
NativeLocalLights::NativeSnapshot localLights; // owns only copied CPU values
HRESULT hr = localLights.Read(enabled, generation, expectedCurrentScene,
                              verifiedLocalLightSources, &liveGeneration,
                              GetTickCount());
if (localLights.Valid(generation)) {
    for (unsigned i = 0; i < localLights.Count(); ++i) {
        const NativeLocalLights::LightValue& value = *localLights.Light(i);
        // Copied data only. Root may later process ALL values in bounded batches.
    }
}
if (FAILED(hr)) {
    // Report LastError() through the caller's always-visible error path.
}
```

`Read` returns S_OK for a new coherent snapshot, S_FALSE when OFF or before the next native attempt is due, and failure for invalid/unavailable sources. S_FALSE while enabled can retain a valid copied snapshot; check `Valid(generation)` before use. OFF resets validity/cadence before any native query. Enabled attempts, including failures, are limited to once per1000ms with initialized unsigned DWORD elapsed arithmetic. Disable/re-enable and generation changes permit an immediate new attempt. The caller schedules this only while visible; hidden/load/new-world/scene loss must call Reset and invalidate the live generation as appropriate. Read/use/reset/release are serialized on the proven render thread. No async consumer may use borrowed native objects.

The count limit is **4096**, with no32-light total cap. A valid4096-entry snapshot retains every eligible light. A4097 count produces ERROR_BUFFER_OVERFLOW before pointer enumeration; it is never clamped/truncated. Root can later process this fixed snapshot in32-light batches. `ObservedCount()` retains the last numeric count for error diagnostics; `Info()` reports native enable/gain/clock and filtering counts. Errors invalidate the entire output and prevent access through Count/Light. `LastError()` is a fixed256-byte buffer; detailed successful summaries belong behind the module's Debug setting.

`LightValue` is112 bytes, with scalar/array fields only: current native scene-world position/range, direction/base power, full float diffuse/specular colours, attenuation C/L/Q, raw custom0 (including floor metadata), packed spotlight cosines/falloff, type and effective power. `Info().SourceSceneOrigin[3]` contains the coherent origin VALUE from that same acquisition. **Positions remain the raw current native scene-node values. No guessed worldOffset/camera correction is applied.** Coordinate registration against the absolute GST2 cache remains a separate native-uniform/integration proof.

The backend's RegistryState records the expected source scene and `SourceSceneOrigin[3]` using that scene's pure native getRelativeOrigin. It verifies the actual12-byte hidden output/return identity, all finite components, and current scene identity before/after the getter. The complete source state is read again after collection: scene identity and all three origin values must match, along with the existing registry/clock fields. A rebase halfway through4096 entries refuses publication and clears the old values/origin. Cached S_FALSE results preserve their original copied position/origin tuple without querying a later frame's origin. No Ogre scene/root pointer is stored in Information.

Only supported active point0/spot2 objects are admitted. Unsupported active types, wrong concrete vtable/scene, absent parent/custom data, invalid field ranges, getter faults or nonfinite data refuse the whole snapshot. Ordinary native invisible/zero-power/day-suppressed/zero-effect entries are omitted with counters. No extra floor rule is invented: native custom0.Y is copied, and the current getVisible/power state remains authoritative. Unloaded/unregistered lamps have no invented state.

## Native ABI and profiles

The backend uses actual504b EXE global0x212cf50, raw array+B0/count+B8 and the owner enable+5C/gain+60 fields. It reads clock global0x212f3c0 and the audited phase/start/end/time fields. It never casts this container to an SDK vector or calls native MainLight. The source header and current scene/generation are checked before and after collection. Native array/object pointers exist only on the synchronous read stack; no Ogre object pointer is retained in the result/backend. The backend retains only module/function addresses and profile-cache scalars.

Heavy loaded module/code-window/export resolution runs **once per generation**, including cached refusal. Per-light work uses resolved functions and a cheap concrete-vtable check. A changed loaded module identity refuses the cached profile; the caller must supply a new valid generation to revalidate. Nineteen exact getter exports,28 EXE ownership/clock/setup windows, three additional Ogre origin-body/sign-mask windows and the concrete Ogre Light vtable are pinned. The loaded game's sine IAT must still target the same MSVCR100 sinf used by the compiled component.

Position and direction getters use their actual hidden aggregate-return ABI: RCX=owner,RDX=caller12-byte output,RAX=output, with output identity verified. Colours/angles/custom data are copied from typed reference returns. Spotlight parameters use the actual native half-angle Cos getter with table=false. No SDK enum/reference guess is substituted. Access-violation/in-page/misalignment guards return explicit refusal; native C++ exceptions continue through normal C++ handling and clear the output. This is not universal recovery from corrupt native objects or concurrent destruction.

The origin getter is actual Ogre RVA2c7af0: RCX=the same expected SceneManager,RDX=caller12-byte output,RAX=that output. It reads scene root0 at+5f0, its local-position SoA at+50/index+38, and negates XYZ. It does not use an SDK root enum, change roots, call setRelativeOrigin or infer a permanent origin history. Runtime pins include its complete negation/output body and the sign-mask constant. Origin is read at most twice per bounded attempt, never per light. Oversized registries still refuse before clock/origin dereference or light enumeration.

The caller's `sourceProfileVerified` must come from its existing background integrity job, checking the exact EXE/Ogre modules plus all exposed `NativeLocalLights::SourcePins` for the current generation. Runtime Read performs **no source/file I/O**. `audit_profile.py` is the separate offline verifier/generator. Native code is assumed immutable within a validated generation; live module replacement/code hooks require revalidation/refusal, not continued use of old resolved methods.

## Native modulation

The copied clock reproduces the audited float32 daytime ramp, including rounded transition endpoints and their subtraction. Native flicker time is `float(doubleClockAtA0 * 60.0)`, never wall time or assumed SharedSkyParams.gameTime. Effect1 and3 retain native sinf/product/widening order; effective power is float(float(basePower*rendererGain)*effect). Unknown finite representable effect codes retain native default1. Day suppression preserves the actual `(custom0.x>0 && daytime>.1)` condition. Sine values are computed once per acquisition, not once per light.

A1Hz snapshot samples native movement/power/effects at1Hz. It does not claim native per-frame animation. A future scalar-clock refresh can be evaluated separately, without repeating registry discovery. No wall/roof/interior/shadow visibility is synthesized. The existing native local-light shader belongs before opaque water composition; its range polynomial and native BRDF still need a separate shader integration/reference suite.

## Verification

Run `run_tests.ps1` with bundled Python, VC10 x64 and SDK7.1. Fresh result: **4335 checks PASS** (4277 clock/collector +58 extracted backend), complete native adapter compile PASS, and **16 independently compiled functional mutants rejected**. Every mutant also compiles the full native adapter; syntax/import failures do not count as negative controls. Generated objects/executables/evidence remain in ignored `_generated`.

The pure-clock oracle copies only the actual SHA-pinned native66c0c0 instructions and9b0090 ramp into a private test allocation, relocates their two constant/two ramp references internally, switches the allocation from RW to RX, and evaluates **synthetic owned clock memory**. It does not load/call the game module or read a running game.4096 phase samples and exact transition boundaries compare real native float behavior against the production policy, rather than a mirrored ramp implementation.

The origin oracle likewise copies the actual pure2c7af0 getter, relocating only its sign-mask reference inside a private RW→RX allocation. Its synthetic SceneManager has distinct root0/root1 data and a nonzero SoA lane index. The actual instructions must select root0, negate all three components, return the supplied output, write exactly12 bytes and preserve the following canary. Those instructions also supply the native getter seam to the extracted production ReadState, so the metadata tests do not mirror the native origin formula. No game module is loaded/called. The missing-origin publication test was observed RED before the implementation (`_generated/origin-red.log`).

The actual templated production collector is tested directly. Actual native backend methods are extracted unchanged, with fixtures supplying only owned raw pages and typed getter/module-resolution seams. Tests cover exact registry/clock offsets, duplicate native entries,4096 admission/4097 refusal, oversized registry before invalid clock dereference, inaccessible/overflowed source spans, vtable/current-scene checks, native point/spot fields and sret identity, hidden/zero-power early exits, native C++ vs SEH faults, whole-snapshot retirement, generation changes, coherent source headers, pause-time preservation, OFF/source guards, init/high-bit/wrap/long-idle cadence, module cache success/failure/recovery and native sine IAT identity.

Mutants enforce: no32-total cap; no silent overflow clamp; no native OFF query; no clock hardcode; exact rounded ramp denominator; native effect3; daytime suppression; correct node-position owner; half spotlight angles; cached heavy profile; current-scene ownership; failed-output retirement; origin publication; origin-rebase rejection; origin aggregate-return identity; and finite origin checks (both validation sites). Origin tests cover all axes, a rebase halfway through4096 reads, wrong expected scene, scene replacement during the getter, a nonfinite origin, overflow before origin access, cached-tuple retention, and a fresh new origin with unchanged raw positions. The initial clock policy was also observed RED before implementation. A fresh audit checks two exact full module hashes,19 exports,28 EXE/three origin windows and seven shader-closure files. The full native adapter object is compiled but never linked/invoked by the tests.

## Work, memory and lifetime

Construction is allocation-free and makes no native queries. On first enabled valid acquisition two4096-value buffers allocate once and are reused:917,504 bytes of value storage. Pointer scratch is32,768 bytes on the synchronous stack; other fixed metadata/profile/error storage is small. This is approximately928KiB of explicit payload including scratch, no GPU payload. `Release()` frees the copied buffers; Reset preserves bounded value-only capacity and invalidates it. A warm production-collector fixture measures zero C++ allocations per read; this does not measure allocator activity inside actual native getters/OS calls.

At most4096 entries are examined per native attempt. One point light uses at most13 resolved Ogre getter calls; one spot uses at most19. Two fixed origin getter calls per successful acquisition add only scalar metadata; value-buffer sizes and the4096 limit are unchanged. No per-light profile check, source read, string construction, growing cache, native ownership change or per-frame discovery is performed. Source identity checks supplement required render-thread serialization; they do not make concurrent native destruction safe or detect every same-count in-place replacement. Expected scene and live-generation storage must remain valid for the synchronous call. Reset on any scene replacement even when a caller has not yet advanced its generation.

This offline suite does not verify actual registered count, runtime getter values, source/camera/GST coordinate correspondence, native render-thread lifetime boundary, full native temporal/HDR image parity, frame time or long-session memory. Separate caller runtime evidence must be assessed independently. The preceding raw-only candidate passed independent4309-check/12-mutant source review. The coherent-origin candidate above passed independent **CLEAR:4335checks/16functionalmutants**, including a fresh complete adapter/profile audit. Capturing this origin stamp closes the missing-metadata issue; it does not prove an automatic absolute-coordinate conversion or replace native-frame/HDR comparison.

Frozen implementation SHA256: `e682c05eb79cf2021de75d2110285841d9d1df12ae13a8b9a5f0e65e135fe275`; generated profile SHA256: `229d5c39b8e0c330893ef81b45e249d97a3fc8dfd430c9f98d4ef5efa119ca5d`.

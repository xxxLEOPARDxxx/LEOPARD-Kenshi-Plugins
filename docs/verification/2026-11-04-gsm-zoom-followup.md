# Reported satellite zoom and display defects, 2026-11-04

Private development build; no public release/version bump or personal release-journal entry.

## Confirmed causes and changes

- x2..x4: finished coarse9m base was shown because the fixed96 fine visible-window ceiling rejected the complete3m footprint. Expand the bounded3m plan to512. Retain the96-window1m threshold, rather than multiplying x8 cold work. x2/2.5/3/4/6/8 actual plans338/210/162/98/45/28 windows on2160x1044.
- Source work previously advanced at most one Main state per workspace frame. Batch already-ready transitions up to8 or1.5ms; unchanged I/O/GPU-wait state immediately yields. Frame shading/presentation remains once. This does not preempt an individual native operation and does not establish a fivefold live speedup.
- Widening residency requires fair shading: two successful shades/frame with initial-unshaded priority and a persistent rotated traversal; repeated lighting epochs must not starve the tail.
- Loaded Hub capture failed its borrowed-resource limits:126004032 texture bytes exceed legacy64MiB; replacing old+fresh buffers needs38314896 bytes. Scope fresh32/128MiB and replacement64/256MiB to loaded towns; preserve legacy limits elsewhere. These are retained native COM references, not newly allocated map texture payload.

## Evidence and limits

LOD904 controls perGPU1/8 andVC10Od/O2; Mainbatch29peroptimization; purepump26/progress12 andnativegetters compile; index6480controls plus4 functional mutants; prefetch8332 andhiddenmap14profiles. Independently reviewed. Actual native metadata/specified CPUallowances67010282B below64MiB; this ledger does not claim to measure every engine/MyGUI allocation.

The screenshot seam followsworldZ3072. All28 current fine source planes are present and numerically continuous. Current shade epochs agree and inspected MyGUI bounds meet atY698, fullUV. This is not evidence that the visible seam has been fixed. A bounded final-output readback will distinguish the lighting/layer output from final UI filtering without recapturing the atlas.

Installed privately at03:59MSK on2026-11-04 after fresh CIM+GetProcess both confirmed Kenshi closed. DLLD4598BA59D2377324C613FB2692A936F16FB04101BD454A5B82426D67D9D0BE5 (1356800B),86native imports/startPlugin/MCM verified. Source05a70ff and94cb5ca pushed. Backup/receiptD:/DEV/Kenshi/backup/gsm-zoom-followup-20261104. PersonalINI A0E2B20E...FED4BC and capture/ready/windows.gsi hashes unchanged. Native seam diagnostic integration independently reviewed CLEAR after source-pin/provenance fixes. Runtime visual and loading-speed acceptance remains pending; no readback request submitted yet.

## Private seam readback

An explicit `fine-seam-request.txt` under the private installed module contains
`id spacing x upperZ`. With Debug enabled, an idle completed view checks this
at most1Hz. Two current same-stamp/same-lighting adjacent final RGBA8 textures
are copied asynchronously; no UI/focus or atlas generation. The one CREATE_NEW
`fine-seam-ID.bin` contains8-byteGSMSEAM1, little-endian32bit metadata length,
UTF8JSON and two tightly packed256x256RGBA8 planes. A short/incomplete file is
not a valid capture. Existing outputs are never replaced. Native WARP tests
verify26 controls,41 injected fault controls and3 functional mutants perOd/O2.

Staging524288 GPUbytes and conservative CPUbytes are charged. Source-output
pins are conservatively charged again until the copy event completes, so a
broken query cannot retain uncharged textures after cache retirement. CPUBlob
is independently charged. Ordinary pending cancellation transfers staging debt
to the existing real-event retirement ledger before Clear; broken requests are
retained with their debt. Known-ready CPUOOM clears safely and reports once.
These diagnostic operations do not by themselves repair the reported seam.

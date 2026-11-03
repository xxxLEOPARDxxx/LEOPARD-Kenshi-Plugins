# Fixed terrain: first actual game-device acceptance

Scope: private four-view base-fixture control, not the production atlas/viewer.
User authorized the terrain rewrite and improved maximum-zoom detail.

## Reviewed and installed input

Adapter freeze SHA256:
`1a67d7174e739851bc05c0fa1d33962c89a4aafb2817620d222b255f0ffd2e06`.
DLL651264B:
`8fe365aba79732327a107e05eb76668315d010db510d66a0b2c817ee743c187f`.
Root verified10 source pins,47 compile dependencies and all166 native imports.
Independent review CLEAR, no P1/P2:
`research/gsm-fixed-control-review-20261103/report.md`, SHA256
`a18c9de18edf300f7971e06a3c9c3383e90a0949c000e2f99c5c9f998df1087d`.

The user closed Kenshi before replacement. Verified backup:
`game/_codex_work/global-satellite-20261103/before-fixed-control-032706/installation.json`.
Only the diagnostic DLL, its16 prepared inputs75,534,626B and temporary control
settings were installed. Viewer startup OFF prevented its189MiB reservation
coexisting with the86,156,268B producer. Active atlas files were untouched.

## Hardware captures

Fresh PID17604 started at03:27 Moscow. User loaded the paused test save and map.
Root requested tokens1601,1602,1603 through INI without focus/input/camera changes.
Each completed four256-square views:1m,3m,9m and3m shifted192m on X/Z. Each
private file is786576B including the144B request/epoch/source-profile header.
All three sets completed, with no fixed-control exception/refusal in the log.
First set was armed at122.706 and reported completion at122.928 game-log seconds;
this is a diagnostic turnaround observation, not an FPS benchmark.

Actual files are under `mods/Codex Satellite Capture Probe/fixed-terrain-17604-*`.
Compact reports:
`research/gsm-fixed-control-review-20261103/game-17604-1601.json`,
`game-17604-1601-overlap-v2.json`, `game-17604-repeat-monitor.json`.
The third set's12 planes are byte-identical to the first set. Completion headers
correctly differ by token, epoch and view identity.

For the3m shared192x192 world footprint, material differs in8 components and
normal in1 component, each by at most one8-bit unit. Reconstructed world-height
difference is at most0.001901m, mean0.000398m. Radial depth itself depends on the
camera, so comparing it directly would be wrong. The production matrix has
clipY=-2Z/span and D3D inverts screenY: the matching slices are first[64:,64:]
and shifted[:192,:192]. An earlier exploratory report used the opposite Y
direction; it is retained as unaccepted evidence, not a terrain failure.

The actual GPU and WARP material/normal outputs are not byte-identical:
per-component mean absolute differences are below0.50/0.29 byte units, with
isolated maxima20/45. Depth differs by at most1.49e-7 normalized units. This
does not establish universal material/filter parity across GPU backends. The
same-device shifted overlap and repeated-capture checks are separate evidence.

## Lifetime and remaining work

All tested jobs joined and emitted completion; process handle count did not grow
across these captures. Private process bytes temporarily increased by roughly
110MiB during repeat jobs. The next pre-job sample returned to the earlier
baseline; later process samples also varied while the rest of the game ran.
These short whole-process observations cannot prove absence of all leaks or
attribute all memory changes to this component. Driver and unrelated game/plugin
allocations are outside the producer payload ledger.

After controls, both personal INIs were restored byte-for-byte from backup.
The diagnostic DLL remains loaded in fixed mode until process exit; no further
token is requested. On the next launch the restored template is not armed.
The current game viewer is still the earlier implementation and atlas.

Next integration uses the reviewed RGB16F raster and streamed world-height
supplier, then generalized exact material banks, shared residency, atomic window
publication and native lighting/city integration. Maximum-zoom detail is not yet
connected to the installed viewer. No public release or personal release-log
entry is created by this checkpoint.

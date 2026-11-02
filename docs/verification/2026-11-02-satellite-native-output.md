# GST2 native output admission and resource completion

Private candidate478208B, SHA256
301b48aa5643af2c790161f017ebec270a843510788ce5be4cc9bad515612aac.
Installed with Kenshi verified closed; backup under game/_codex_work/
global-satellite-20261102/before-native-output-fix/installation.json.
Runtime display acceptance is still pending; no new terrain capture is needed.

## Reproduced draw refusal

The first game run verified the complete1519 atlas/source profile, then returned
E_INVALIDARG on the first relight. Actual native Ogre creation code sets
GENERATE_MIPS1 on its ordinary one-mip SRV|RT output. A real WARP allocation with
that descriptor reproduced80070057 before the repair; it now shades to the
same pixels as the private output fixture. Only Misc0 or exactlyMisc1+Bind0x28
are accepted. Cube/shared/keyed flags, extra mips and all existing dimensions,
format/device/usage/CPU/ownership/alias checks remain restrictive. No mip
generation, extra allocation, shader/light or budget change was introduced.

The helper reports a bounded scalar refusal stage; descriptor text is built only
on an error. The user notice distinguishes a verified atlas that could not be
drawn from unavailable input. Both leave the ordinary map available.

## Managed image completion

The actual backend override called by TextureManager::loadImage credits its
creator once but omits Loaded. Native setToLoaded66740 only writes state2;
isLoaded66750 checks that state, and creator666D0 identifies the manager.
After a successful manager call, the adapter checks profile, concrete texture
type and exact creator, then settles Loaded only when false and verifies it.
It never sends another manager notification. Failed manager calls do not enter
completion or SRV adoption. This prevents the known ordinary load/unload
accounting mismatch without changing the creation path or adding frame work.

## Warning and catalogs

MCM v4's supported custom area hosts one red, wrapped, noninteractive warning:
at least24GB free before full4x4/8192 rendering; about23GB uncompressed data.
It reuses one lifetime-tracked child and removes it on page detach. Older API2/3
has a plain tooltip fallback. No disk polling or new setting was added.
The build compiles exactly the three module PO files, preventing stale MOs.
EN/RU/ZH each contain14 messages. Detailed log remains the last setting.

## Verification and limits

- Actual native-shaped WARP regression RED then GREEN;92 WARP checks and six
  separately compiled negative controls pass.
- Ten extracted production completion/Upload checks cover successful/unloaded,
  already-loaded, failed manager return, profile/type/creator rejection, inert
  setter and repeated completion with no extra credit.
- Viewer suite:13 metadata,55 real worker/candidate,17 extracted geometry,
  13 clock and128 native sampler checks pass; current source count222.
- Build, decorated startup, MCM export and77 native imports pass.
- Independent review CLEAR: research/gsm-output-descriptor-20261102/report.md;
  final report SHA758950006bff66f0346ecafb9712bed645b646859512a1409d84d9f9b67bc5e9.

No live display, native/modded skin warning layout, sustained FPS, whole-map
seams or long-session leak claim is inferred from the tests. All16 finalized
atlas files are retained. Generator startup is disabled for the next game run,
while its empty FCS wrapper and mods.cfg remain unchanged for the verified
source profile. The private viewer and DF are not public releases.

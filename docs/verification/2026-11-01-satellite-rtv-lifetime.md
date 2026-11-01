# Private satellite RTV lifetime: native repeated-capture proof

This repairs the observed approximately 3 GiB retained per 8192 capture. It does
not complete detailed terrain, buildings, dynamic lighting or global-map integration.
The existing minimap is unchanged. No public mod package/version is changed.

## Native cause and ownership

Installed RenderSystem_Direct3D11_x64.dll SHA256:
`5f224882ded5b6d3f68f14d6bb4e1762267bbb602b7b9733bd153663d1541bb9`.
The profiled D3D11RenderTexture constructor/rebind owns the reference returned by
CreateRenderTargetView, stored at +F8. The getter returns it without AddRef; both
observed destructors omit Release. Six independently owned private targets total
3 GiB at 8192. Disk and loaded getter, destructor/vtable and relevant body windows
are guarded. No native DLL is modified and no global rendering hook is added.

The production six-entry debt registry pins both TexturePtr and pixel buffer.
Cleanup first removes workspaces, material bindings, manager entries and ordinary
plugin references. All six identities/reference counts are preflighted before
any final destruction. After synchronous RTT destruction, each creation debt is
marked spent before exactly one COM Release. Both successful requests below
reported remaining=0 for all six. Uncertain native constructors, changed aliases,
context or counts quarantine one bounded set and prohibit further captures.
There is no global ClearState, Flush, cache eviction or heap trim. Metadata has
no late automatic destructor that could run after Ogre Root destruction.

## Verification

Production helper checks: 56 policy and 92 RTV ownership cases, alongside the
existing native ABI, attachment, pose, terrain, worker, exposure, raster and GPU
shader checks. Exact installed module profiles, loader exports and 119 native
imports pass. Independent review identified two issues that were corrected:
guard the dynamic deleting-destructor slot/body as well as the getter; cancel a
pending controller token before evidence IO on every exit.

Native evidence root (private, not distributed):
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261101/rtv-lifetime-20261101`.
Each request has capture.json, native log excerpt, sampled memory and original
BMPs. No application was focused or controlled automatically while the user
worked on their article.

Two 512 captures used DLL SHA256
`350202660e10e4a1d94fdbf16afb22fd5bbced329fd7116797b0ee49ae0710c4`.
Both completed six releases; GBuffer and linear-clamped Sun BMP pixels match exactly.
Exposed output differs by at most one level. The first controller elapsed time
includes waiting for manually supplied foreground; native capture/cleanup took
1.262 s. An earlier token timed out before allocation and was cancelled.

The 8192 DLL SHA256 is
`c8183d9a35e0ea74063f77786f7b0f0b47ace3f5dc532ab4a7086f705fad6eb2`.
It adds explicitly opted-in background file requests. Keyboard requests remain
foreground-only. Startup default AllowBackgroundFileRequests=0; the controlled
private installation sets it to 1. Requests require an open map and valid idle
render context and do not replay the token present at startup.

Process 8124, start UTC 2026-11-01T10:12:33.5156518Z, same paused SAT scene:

| Measurement | First request 1401 | Second request 1402 |
|---|---:|---:|
| Controller elapsed, including proof IO | 8318 ms | 7458 ms |
| Private bytes before | 7,763,996,672 | 8,066,093,056 |
| Private bytes immediately after | 8,437,628,928 | 8,341,356,544 |
| Private bytes after idle | 8,065,966,080 | 8,078,729,216 |
| Idle handles | 14,579 | 14,579 |
| Released creation references | 6, all remaining 0 | 6, all remaining 0 |

Second idle is 12,763,136 bytes above first idle, not another approximately 3 GiB.
The initial warmup retained about 302 MB of process memory; these short controls
do not prove absence of every leak or long-term FPS impact. Whole-GPU used memory
was 4896 MiB before capture, 5146 MiB after first idle and 5057 MiB after second
idle. This includes the browser/other applications and is not a per-process GPU
allocation measurement. The native shared depth pool may retain its first target.

Each request saved three genuine 8192x8192 BMPs, 268,435,510 bytes each. Both
67,108,864-pixel position and raw Sun readbacks were finite. GBuffer and
linear-clamped Sun BMP pixels match exactly (not a full HDR float-array comparison).
Exposed images differ at 1693 pixels, max channel delta34,
mean channel delta0.0000113894542; adaptation is read separately per request.
Geometry remains 2048-equivalent (detail32), distant colours remain coarse.
The 1 GiB native geometry and 16M estimated index limits were preserved.

The controller cancelled Request to 0 after both captures. Game remains open in
the background with its map visible; no further capture runs automatically.

## Next work

Replace coarse DistantTerrain sampling with independently owned detailed biome
inputs, retaining actual installed/modded content and avoiding global ZoneMap
callbacks. Then prove mesh boundaries, building/LOD coverage, water/shadows and
day/night rendering, and integrate the optional global-map viewer. DF is paused;
KEO follows the completed satellite feature.

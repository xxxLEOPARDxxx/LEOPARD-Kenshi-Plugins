# Canonical DDS native-group fix: independent review CLEAR

2026-11-03. Read-only production review. No game input/files/configuration, native calls, production edits or commits by reviewer.

Frozen `GlobalSatelliteMap/atlas-terrain/world-native/SourceFactory.h` SHA256: `eb99bc837c802509585fbbf333eabb374cd666a11d8560d7c6ea26f495de8155`.

No open P1/P2 in the requested small group-routing correction.

The two pinned catalogue DDS entries under exact `data/newland/assets/textures/` are registered in native **General**, not Landscape. Native group basename indexes are separate; the old uniform Landscape request selected a different vanilla file of the same size/name and correctly failed canonical worker SHA validation. The actual `resources.cfg` registers `./data/newland/Assets/Textures` in General. Existing Landscape/Newland remapping remains unchanged.

The new bounded pure helper recognizes only the two exact pinned catalogue parent namespaces: Assets/Textures→General and land/textures→Landscape. It does not scan folders, hash files on Main, enumerate native locations or change archive resolution. All219 catalogue paths have a defined partition,2General and217Landscape. Unknown, nested, traversal or similarly prefixed parent namespaces refuse. Overlaymaps' three inputs and the blendMap name special case are unchanged.

Request and ValidateSourceName use the same explicit group. SourceFactory::Matches regenerates the actual canonical request, so an old/wrong-group snapshot cannot pass the routed Refresh path. Asset roles, pinned byte counts, canonical SHA256, full64bit epochs/scope and resource basenames are unchanged. NativeMetadata, worker lease, native profile, listener policy and NativeCompatible semantics are unchanged; actual registered archive identity and physical canonical file SHA remain independently enforced by existing gates. This does not add a universal cross-archive basename collision resolver.

## Independent focused controls

`review.cpp` includes the real SourceFactory header and executes real WorldMaterial plans/SourceFactory requests for regions0,2 and27. VC10x64 **/Od and /O2 each pass316 additional controls**. The tests actually reach75requests:2General,58Landscape and15Overlaymaps. Every request's independently known catalogue partition is checked, its wrong group is rejected by the real Matches route, and source/pin identities retain full64bit values. Nine invalid/unknown/nested/traversal/prefix namespace cases refuse. PE imports are inspected before execution; no Kenshi/Ogre/D3D/UI DLL import is allowed. Test source and logs are beside this report; there is no fixture file deletion or game read/write in these additional controls.

Author physical462-control tests were reviewed: the actual same-size wrong Bone DDS file fails production WorkerFileLease SHA, the correct canonical file succeeds without early data exposure, and a changed archive identity withdraws consumption through the real MainGate Fresh flow. The test provider supplies copied metadata and is explicitly not actual native registry evidence. Existing world-native122105/Od+O2 and native compile controls were also recorded by author; four runtime mutants pass the expected rejection checks.

## Acceptance boundary

This CLEAR establishes the exact source-group mapping correction and unchanged safety checks. The root still must prove the actual game's **General** indexed winner for region2/cell0 and then complete the all-region render. Neither author fake metadata tests nor this pure review constitute that hardware/native-registry acceptance. No full-map/seam/detail/FPS/leak completion is claimed.

# Canonical source resource groups

The first full native build stopped at region2/cell0/stage1/input3 with
E_INVALIDARG. That role is the authored Assets/Textures bone diffuse source.
The catalog contains two such Assets/Textures DDS files, registered in the
General resource group. The other217 DDS assets belong to Landscape. The old
factory routed every DDS basename through Landscape.

Vanilla contains two BoneLoop_DIF_WHITE.dds files with identical5,592,560-byte
sizes but different pixels/hashes:

- `data/newland/Assets/Textures/BoneLoop_DIF_WHITE.dds`:
  `ec0e13f7d023eec1ac9842a0553f2caaf28dc0c3ad0622175214b00e4a5d0457`
- `data/newland/land/textures/BoneLoop_DIF_WHITE.dds`:
  `503ea826ae035bc92fea45038a8fe3c497bcfe9cd21affcbef9dc13c8666b91b`

`SourceFactory::DdsGroupForCanonical` selects General only for the exact pinned
`data/newland/assets/textures/` parent and Landscape only for the exact pinned
`data/newland/land/textures/` parent. Unknown, nested and traversal prefixes
refuse. Request creation, name validation and factory matching use the same
canonical group. PNG Overlaymaps routing, role IDs, asset paths, sizes, SHA pins,
NativeMetadata, NativeCompatible semantics, archive selection, worker IO and
fresh archive checks are unchanged. No wrong file is accepted by changing a hash.
The helper is bounded string metadata only: no allocation, IO or engine call.

`run_tests.py` compiles actual SourceFactory/MainGate/WorkerFileLease production
code under VC10/Od and/O2, with no engine DLL imports. Each passes462 controls:
all219 catalog paths, exact2/217 group partition, the real failed role and normal,
actual resources.cfg General registration, invalid namespace prefixes, and a
physical same-size collision. The test-owned metadata provider models the two
explicit archive namespaces; it is not a native Ogre runtime assertion. The
wrong Landscape file fails the unchanged worker SHA check. The correct General
file opens and becomes accessible only after the actual fresh MainGate acceptance.
Changing the archive identity still withdraws access. Native game confirmation
remains a separate control by the parent agent.

The observed pre-fix RED is `_generated/red-group.log`. The focused corrected
suite and existing complete world-native adapter suite passed; the latter covers
122,105 controls per optimization,29,985 requests and four runtime mutants.
These checks do not prove the full atlas is complete, material continuity, FPS,
long-run memory behavior or every future mod's archive layout. Any unpinned
resource winner continues to refuse under existing identity/hash guards.

# Raw indexed owned sources and native terrain names — 2026-11-03

The minimal explicit `OwnedRawIndexedFile` policy is justified for a **new
plugin-owned file input**. It does not establish equality to native delivered
streams or historical GPU textures. Keep the default `NativeCompatible` policy
strict about a null listener. No listener, `openResource`, native stream or game
callback should run on the copied-data worker.

This was a read-only audit. No source under `ours`, game state, UI, installation,
settings or Git history was changed. Existing earlier lifecycle test scaffolding
in `world-stream` belongs to the previous assignment; this assignment did not
modify it. Evidence here consists of local source reads and ReadProcessMemory
of running Kenshi PID29860. The process disappeared before the optional final
helper trace; no relaunch was attempted.

## Why a non-null listener is normal here

`D:/DEV/Kenshi/forks/RE_Kenshi-src/OgreHooks.cpp`, revision
`c8dfbc9621b9263863722852f819c4c0f440475a`, explicitly says the game already has
a DDS loading listener (line16). `InitFinalizeMods` installs `TextureDownscaler`
around that existing listener at line136. Thus null-only acceptance is expected
to refuse an ordinary RE_Kenshi game, including at maximum texture quality.

`resourceLoading` lines27–40 can delegate to the original listener or return an
empty stream to let normal archive opening proceed. `resourceStreamOpened`
lines68–117 can drop DDS mipmaps, replace the stream with a new memory stream,
fall back to the original listener, or delegate non-DDS inputs. The condition
`texture == resource` also admits Resource=NULL (line73); an Image::load caller
cannot evade these semantics merely by lacking a Texture resource pointer.
`Overlaymaps` DDS receive a separate mip-drop exemption at lines84–85, but PNG
requests still delegate to the original listener. No equality claim follows.

The previous exact Ogre audit at
`../gsm-native-source-winner-20261103/report.md` establishes that
openResource first offers `resourceLoading` the request and later offers
`resourceStreamOpened` the archive stream. The archive index therefore proves
only raw indexed-file selection when a listener exists.

## Exact game callsites and names

`group-refs.json` records the live image base, process ID, matching strings and
24 RIP-relative string references. Adjacent `game-*.asm` are the corresponding
live function bodies. The installed disk executable has different section and
string RVAs, so the final audit uses **live PE section headers and live pdata**;
disk offsets were not substituted for live callsites. The collected .text hash
is a snapshot fingerprint, not a production compatibility pin. Several live
function entries already contain detours; the original surviving callsites
quoted below are present and individually readable.

| Input/path | Actual caller evidence | Consequence |
| --- | --- | --- |
| Biome DDS diffuse/normal arrays | `ZoneMap::createTextureArray`, RVA0xa10080.0xa10111 initializes the static string `Landscape`;0xa10118 points to static object0x2134dd0.0xa10942 calls `UtilityT::removePathFromString`;0xa10953 passes that same static object as R8, and0xa1095d calls `Ogre::Image::load(name,group)`. | The logical DDS name is its **basename**, group `Landscape`. A pinned canonical `data/.../file.dds` is not this request name. |
| Overlay object directory/group | RVA0x408c59 constructs `Overlaymaps`;0x408c7e constructs `data/newland/land/overlaymaps`;0x408c8f–0x408ca5 passes both separately into the common image constructor. Ambient/distant image constructors0xa0ef50/0xa0f860 also construct the same separate directory/group. | Physical directory and logical group are distinct inputs. `Overlaymaps` is supported as an intentional group for our owned source; this alone does not prove a historical texture's resolved group. |
| Overlay and colour names | `ZoneMapOverlay` constructor0xa136e0 builds the numeric dotted suffix.0xa1396b passes `new_overlay` plus the suffix and `png` to image construction.0xa139e3–0xa13a15 constructs `colour` + suffix + `.png`. | Use the captured logical name, not an arbitrary canonical catalog path. Final get-filename helper0x43cc0 was not fully traced before process exit. |
| Native terrain material overlayMap | `ZoneMap::getTerrainMaterial_DX11`,0xa157c2 gets the overlay filename;0xa157e5 fetches `AUTODETECT_RESOURCE_GROUP_NAME`;0xa157ec places it in the fifth argument;0xa157fa calls `ResourceLoader::loadTextureUnit`. | The material path uses **AUTODETECT**, not a literal `Overlaymaps` argument. |
| Native terrain material colourMap | Same function:0xa15894 fetches `AUTODETECT_RESOURCE_GROUP_NAME`,0xa1589b sets fifth argument,0xa158a9 calls `loadTextureUnit` with the stored colour name. | No canonical-path-to-native-group equivalence is proven. Observe the resulting resource's actual name/group, or separately support/profile the actual group-search semantics. |

`resources.cfg` additionally has separate `[Landscape]` and `[Overlaymaps]`
sections, with `data/land/textures` and `data/land/overlaymaps` respectively.
Newland paths are also constructed in code as above. A static configuration
section cannot replace the current indexed winner captured on Main; mods may
add or replace locations. The earlier Ogre `findGroupContainingResourceImpl`
assembly shows an ordered current group-map walk, not mod-list order. Guessing
the first matching group from five canonical-name aliases is diagnostic
enumeration, not evidence of the exact native request route.

For the first fresh owned-source job it is honest to **choose**
`Landscape`+DDS basename and `Overlaymaps`+PNG basename, require that precise
indexed pair to exist, and authenticate its actual file bytes against the
expected pin. Document this as our own raw source contract. If a pair is absent,
refuse; do not silently try other groups/aliases and label the result native.

## Read-only review of the new provider change

Reviewed current `source-receipts/MainGate.h` and `NativeMetadata.h` after the
parent's change. No blocking defect was found in the scoped semantics change:

- Default policy remains `NativeCompatible`; invalid policy values refuse Init.
- Raw mode only binds `OwnedNamedFile`, with no retained native resource.
- Main-only snapshots stamp both semantics and the listener pointer.
- Snapshot reads the listener before and after index/path capture, comparing it.
- `SameSnapshot` compares both fields during acceptance and every bank refresh.
- `BeginBank` requires every receipt to match the provider's semantics and the
  one current listener while holding manager/group locks.
- WorkerRequest remains copied plain data; the listener is never dereferenced
  or called by this provider, and no worker obtains native identity pointers.

A listener can internally change behavior without changing its pointer; that
does **not** invalidate this raw-file contract because no listener-produced
bytes are claimed. It would invalidate an attempted native-stream equality
claim, which this mode must continue to avoid. Pointer identity checks are a
conservative source-scope invalidation signal, not a proof of listener behavior.

This review is static. It does not replace the parent's independent lifecycle
tests or an actual Main callback control of the changed provider. No FPS,
long-running leak, finished streaming feature or native-texture equality claim
is made.

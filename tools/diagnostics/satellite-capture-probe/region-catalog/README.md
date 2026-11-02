# Fixed regional geography catalogs and explicit adapters

This step generates64 **data regions** of36864m, retaining the final16 logical
primary8192² cells. Root subsequently authorized `BeginRegion`/`BeginPrimary`
adapters and an explicit-origin shader overload. CaptureProbe/TerrainExperiment
orchestration, atlas format/output and viewer belong to root; this implementation
does not edit their files, release metadata, installed DLLs, INI or game files.

## Concrete design

- Pin the existing world snapshot/active records and installed asset identities.
 Validate all219 DDS paths,128 painted PNGs, global blend/fullmap and native
 executable/module/shader identities before writing generated output.
- Store57 global SID identities separately from at most15 local shader rows.
 Every coarse cell retains all five ordered global source slots and its exact
 tree byte count/hash/root mask. Five regions contain unused non-null source
 slots; these must survive even though no local shader row/texture is allocated.
- Each region has16 coarse cells,4×4 active selector texels within the existing
8×8 resource, its exact world origin and its parent primary's painted-map base.
 Preserve native descending slot order and original channel numbers.
- Keep exact24/22/46 BC bank shapes. Map six roles through content hashes while
 retaining each SID's actual source path (including aliases). Unused bank layers
 deterministically repeat the first used source of that format; unused local
 biome rows are invalid sentinels, never fake GameData identities.
- Emit bounded C++03 immutable structures/data and a small pure validator/lookup.
 No Ogre/D3D types, API calls, retained engine pointers or runtime allocations.
 Source-file verification remains offline; future integration must additionally
 validate live identities and ownership before GPU allocation.

## Implementation/verification plan

1. RED: executable tests for generator outputs, all raw slots, deterministic
 padding, boundary ownership, path/hash/shape refusal and unchanged outputs on
 failure. Compile C++03 tests against the desired data/validator API.
2. GREEN: implement the bounded generator and schema/validator in this directory.
3. Check all1048576 actual fine-leaf selections against the existing native-
 derived `BiomeSelection.h`,63488 coarse boundaries and all64 catalog edges.
4. Compile with installed VC10/SDK7.1, run generated-copy mutation checks, rerun
 deterministic generation and report source/output hashes, memory and timing.
5. Root reviews this isolated result before any active-render integration.

All writes and build outputs stay inside this directory. Existing source/render
assets stay unchanged. The authorized adapter changes are listed in
`verification.md`; no automatic cap increases or new helper framework are part
of this step. Run `run_tests.ps1` for the complete new suite; use the parent probe
`build.ps1` for the full historical build/import/GPU regression suite.

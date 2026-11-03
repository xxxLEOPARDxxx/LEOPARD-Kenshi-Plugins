# Independent concrete disk-factory review

Verdict: CLEAR for the final frozen worker-only factory. No remaining confirmed P1/P2 finding and no production correction requested. Parent integration/borrow rules and the current-game test remain separate.

Manifest SHA256 `09c183c231e52064414822a5e10da4614804885f69590830dcd3185a347ee2c1`; WorldBuildDisk.h `78a93d3bc2f82b181c1e3897f4316379aebd1d1ac175bd3e4d0fd1280605e858`. All280 original source/dependency/log/artifact/cleanup pins were verified before and after the isolated tests.35 project inputs were mirrored. Original/frozen source and outputs were not edited.

## Boundary and corrected races

Configure is pure bounded text copying: two owned221-wchar fields, local drive-absolute spelling, normalized separators, one immediate child of the caller-selected existing parent. It rejects traversal/dot/empty components, device/UNC/extended prefixes, ADS, reserved DOS names, trailing dots/spaces and overlong paths. This is not discovery of an authorized game directory: the checked Main caller must select the intended parent. No external pointer survives in Config.

The final factory opens and retains ancestor/parent directory handles, verifies resolved DOS paths and nonreparse directory attributes, and creates its child atomically through NtCreateFile(FILE_CREATE) rooted in the held parent. FILE_CREATED plus the returned retained handle is required; an existing directory cannot be adopted between CreateDirectory and a later open. Files likewise use fixed tile leaf names and FILE_CREATE relative to the retained output-directory handle. There is no full-path tile reopen after the guard check.

The prefreeze sharing/namespace concerns were concrete. Attributes-only handles did not establish the intended sharing protection; data/list access was added. More importantly, metadata-only access can still set a reparse point despite sharing flags. The decisive final race uses FILE_WRITE_ATTRIBUTES; the zero-access attempt is not represented as a universally successful mutation. The final code/docs correctly avoid claiming sharing prevents all metadata changes.

An actual mountpoint retarget inserted after the guard check makes relative file creation safely refuse on this machine, with neither original nor target entry created. Restoring the fixture metadata permits creation in the original directory. The test also compares retained directory/file identity and kills a full-path CREATE_NEW mutant that writes to the redirect target. Constructor and post-create attribute checks fail closed on observed reparses; relative creation is the confinement boundary through the race.

The NT function signature/types/flags are the SDK winternl contract, dynamically resolved from the already loaded system ntdll. These real OS filesystem calls were tested. No Ogre/Kenshi/native-game contract DLL was loaded or executed. The implementation creates at most one directory and16 uniquely owned tile files per factory, exclusively; it does not replace a tile, auto-delete a forensic partial, resize an atlas from Main or publish COMPLETE/ready metadata.

## Ownership and resources

Factory construction and all its file calls require the dedicated creating worker. The file interface checks its creator for Resize/WriteAt/Flush; OpenNew checks creator/index/duplicate state before creation. Session/Worker must destroy all files before Factory on that worker. Destruction is nonthrowing and closes owned handles. The factory is not itself a detector of the real game Main thread: the existing writer wrapper enforces where it is constructed.

The new HANDLE-backed RandomFile is needed because the frozen Win32File cannot adopt the atomically created relative handle. It owns24 bytes in this VC10 x64 build, checks extent before seek/resize, handles partial WriteFile progress, flushes and closes. Its allocation-failure path closes the already created native handle and preserves only the new forensic leaf. No data vector, retained input, per-row allocation or growing queue is introduced. Factory/config/112-handle array size is charged by WorldWrite::FixedCpuBytes; Session reserves all16 file objects via the exact24B bound. OS handle-table/stack/allocator costs are not a total-RSS guarantee.

## Fresh evidence

- Full frozen suite in an isolated mirror: UNICODE `/Od` and `/O2`,169 worker-only controls each,16 actual namespace-retarget controls,169 forced-short-write controls. The three path-open/short-loop/tile-bound mutants fail their intended runtime assertions. Host imports were checked before every executable run and exclude game/GPU/UI DLLs.
- Added100 controls each optimization: copied Config cannot redirect an existing factory after external mutation;220-character inclusive/221 refusal, unterminated/null and additional reserved/path cases; overflow/null bounds; foreign factory/file calls refuse; valid actual bytes persist;64 failed constructor cycles return to the same process handle count; final factory destruction releases directory rename protection.
- Added27 allocation-fault controls: after exclusive native file creation, a forced File object allocation failure returns ERROR_OUTOFMEMORY, closes its handle with unchanged process handle count, and leaves only the zero-byte owned forensic file.
- All normal/fault/race/mutant fixtures are fresh research-owned roots. Inventories and manifests are retained, reparse targets are validated and not followed, and cleanup removes individual owned entries. No fixture directory remains. Files are at most4096 bytes; no full1.43GB GST2 resize occurred.

Reproduce with `reproduce.py` then `extra.py` using bundled Python/VC10/SDK7.1. All additional code/logs/manifests stay under this research directory. The guarantee is confinement of operations through the retained objects under the tested Windows filesystem behavior, not prevention of arbitrary privileged filesystem mutation, durable power-loss recovery, full-file throughput, all OS faults, long-run leak freedom or atlas completeness. The containing parent must still retain borrowed owners until writer acknowledgement/known join, and retain the whole job/debt on quarantine.

No game/UI/install/INI/commit operation was performed.

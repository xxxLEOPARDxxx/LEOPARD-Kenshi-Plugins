# Concrete worker-owned partial atlas files

`WorldBuildDisk.h` implements the CPU-only WorldWindow::Factory interface for the
existing WorldWrite worker. Nothing in this helper touches a game object, native
source provider, GPU, UI or Main callback. Configure only copies/validates text;
the Factory is constructed inside the existing writer worker, where all directory
and file operations occur. No frozen helper or caller is changed.

API is `WorldBuildDisk::Config`, `Configure(config,allowedParent,newDirectory)`,
and `WorldBuildDisk::Factory(config)`. Config owns two221-wchar arrays, allowing
at most220 path characters plus termination, and has no external pointer. Paths
must be local drive-absolute; slash spelling is normalized. The output must be
ONE immediate child of the explicitly configured allowed parent. Relative, UNC,
device/extended prefixes, dot/traversal components, ADS, empty components, DOS
device names, trailing dot/space, nested children and overlong paths refuse.
The caller supplies the intended existing parent, for example the private probe
mod's directory and its new world-build-pid-token child. This is an explicit
caller-selected root, not an implicit permission for another filesystem tree.

Factory validates every existing ancestor and the parent with retained directory
handles, resolved DOS paths and no reparse attribute. Handles omit write/delete
sharing and prevent ordinary data-write opens and rename/deletion. **Metadata-only
handles can still set reparse points.** Sharing is not a blanket namespace lock.
Directory creation therefore uses Windows NtCreateFile FILE_CREATE relative to
the retained parent handle, obtaining the NEW directory and its retained handle
in the SAME operation; FILE_CREATED is required. There is no CreateDirectoryW /
subsequent-open adoption gap. Directory identity/path/attributes and retained
ancestors are checked again before exposing the factory. Existing output paths
always refuse, including empty directories and existing reparse points.

OpenNew(x,z) admits only0..3 per axis and each tile once. It creates exactly
`tile_x_z.gst.partial` using FILE_CREATE relative to the retained output-directory
handle; it does NOT resolve the directory path again. Existing files/reparse
leaves cannot be adopted or overwritten. A reparse change detected before/after
creation refuses; a raced relative creation cannot follow the path into another
directory. The actual retarget control inserts FSCTL_SET_REPARSE_POINT between
the guard and file creation: on this machine the relative NT operation refuses,
with no original or target entry created. Restoring ONLY the test fixture's
metadata then allows safe creation in the original directory. A deliberately
path-based CREATE_NEW mutant creates a file in the redirect target and is caught.

The immutable file HANDLE is then transferred to a uniquely owned RandomFile
adapter. Root explicitly authorized this own-handle adapter: frozen
Gst2::Win32File cannot adopt an already atomically created relative handle, and
its path-based OpenNew could not provide the required namespace guarantee.
The new adapter preserves the bounded seek/short-write loop/flush/close behavior.
Resize and offset+length are capped at Gst2::FileBytes(), well below signed64-bit
overflow; invalid bounds refuse before IO. All operations run on the creating
worker thread. FileCpuBound is EXACTLY sizeof(File),24B in the tested VC10 x64
build. No data vector or growing queue is owned. Factory's copied config and
fixed112-handle guard array are counted by WorldWrite::FixedCpuBytes; session
accounts at most16 transferred file owners through the24B bound.

Factory construction throws a Win32 HRESULT on invalid/unavailable/non-new paths;
WorldWrite's existing worker catches construction failure. OpenNew returns NULL
on refusal/failure. Destructor/partial construction close retained handles with
no exceptions. Files must be destroyed before Factory, as WorldWindow/WorldWrite
already do. No file or directory is ever auto-deleted by production: failed jobs
leave forensic partials. No large Resize occurs until the path/NEW-only checks
and exclusive relative file creation have succeeded. This helper does not write
completion metadata, close an atlas publication transaction or claim readiness.

`python run_tests.py` uses VC10 x64 /MT /DUNICODE /D_UNICODE under /Od and /O2.
PE imports are inspected BEFORE host execution and must exclude game, GPU and UI
DLLs. Concrete Factory IO occurs on a test worker. Controls create only fresh
owned fixture roots:16×4096B tiny files, one4096B collision file, real junction
positive/negative controls, actual metadata-only reparse retargets, held rename
and write-handle refusal, indices/duplicates/path/length/existing-parent checks,
exact bytes/readback/close and retained forensic partials. No GST2 Session is
started, no full1.43GB file is resized and no atlas-sized fixture is allocated.
Additional controls force short writes and kill path-open, short-loop and tile
bound mutants. Test artifacts are inventoried, confined by absolute paths with
no reparse ancestors, written to a cleanup manifest, rechecked, and removed
individually without recursive deletion or following junction targets.

The guarantee concerns writes performed through this factory into its retained
directory objects. It does not prevent a privileged actor from modifying the
filesystem, establish durability after power loss, total RSS, disk throughput,
all OS failures or safe reuse after an unjoined writer. Main must keep the entire
outer owner alive until the existing writer is known joined; unknown waits keep
that owner quarantined. No UI/game/install action is part of these controls.

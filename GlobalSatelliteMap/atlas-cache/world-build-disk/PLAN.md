# Worker-owned new atlas files

New directory only. Config copies an explicit allowed parent and immediate new
child directory, at most220 wchar path characters each. Configure is pure; all
filesystem validation/creation is in the existing writer worker's Factory.

- [x] Pin pure path, new-directory/new-file and tiny readback controls.
- [x] Hold checked ancestor/output directory handles, then atomically create
  directories/files relative to their handles. Root authorized own HANDLE file
  adapter because frozen Win32File cannot adopt a relative-created file handle.
- [x] Test indices, duplicates, existing/unsafe/reparse/long paths and cleanup.
- [x] Freeze exact source/dependency/import/test evidence and request review.

Author controls pass; independent review pending. Share modes do NOT block every
metadata-only reparse operation; real retarget race containment is tested.

No GST2 resizing in the backend tests, no Main/game/GPU/UI operations and no
automatic deletion of forensic partial output. Test cleanup uses its own fresh
fixture inventory/absolute-path manifest and never follows a reparse target.

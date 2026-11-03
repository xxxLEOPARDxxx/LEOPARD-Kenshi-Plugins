# Accepted-window adapter

Scope: a serialized worker-only CPU adapter in this folder. No scheduler, IO, writer, native integration or edits to frozen dependencies.

1. Add actual Accepted fixture tests and a minimal unimplemented adapter; observe a runtime RED on valid Build.
2. Implement one owned normalRGBA8 copy (maximum1MiB); preserve RGB and set coverage alpha255 only after Accepted.Acquire. Borrow materialFP16/gloss and worldheight unchanged.
3. Fresh Acquire requires the original owner object, exact scope/frame/authenticated height window; compare original normal bytes with the prepared copy to refuse changed same-frame owners. All refusal/cancel paths clear output. Source owner/window must remain alive and unchanged while a View is consumed.
4. Auto-charge accepted owner, height window and adapter; explicit ledger adds retained raw storage and other payloads. Use subtraction-based64MiB admission before allocation and on Acquire. Cancellation before allocation/every row/final publication.
5. Test fourteen real256/512 frames, writer512 compatibility through a memory-only RandomFile sink, stale/source/OFF/cancel/budget/owner reuse, allocation/cancel controls and meaningful compiled mutants. Test existing offline coverage mip behavior without changing it. Freeze own source, dependency and fixture hashes.

Ruling: support256 typed views for fine windows but never resize/resample them into a512 writer request. Source continuity and atlas pixel-coordinate assignment remain the caller's responsibility. The existing GST2 format is already decoded FP16 RGB/gloss8B +normal4B +height4B, flags0.

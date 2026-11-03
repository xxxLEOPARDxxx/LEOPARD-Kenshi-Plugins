# Owned satellite source inputs —2026-11-03

The installed map/atlas remains unchanged. This stage admits source metadata for
new owned render inputs; it does not prove historical native texture equality,
successful worker reads, a completed new terrain frame or long-run performance.

## Actual Main controls

- Strict-null control1701, PID29860: exact native profile and catalog passed;
 16groups/80attempts, zero accepted metadata. Read-only memory inspection found
 the normal RE_Kenshi TextureDownscaler listener. Refusal was correct for this
 strict policy; the new renderer needs original file bytes rather than the
 listener's potentially mip-reduced image stream.
- Explicit raw-indexed control1702, PID30220: profile/catalog passed,
 15groups/75attempts, exactly one metadata match: group `Overlaymaps`, logical
 name `new_overlay.3.3.png`, installed `data/newland/land/overlaymaps` file.
 This is MainGate::Capture only. No source file, GPU allocation or image write
 was performed by either metadata request.

The audit is startup opt-in, token-latched, checked Main/world/map/save-idle only,
and bounded to32groups/160attempts. Default controls remain off. Ordinary map
operation does not scan these groups each frame. Personal probe INI is backed up
under game `_codex_work/global-satellite-20261103/before-native-receipt-0635`;
restore its exact bytes after the control session. Per-run logs/installation
hashes are in the sibling `native-receipt-live` directory.

## Source contract

`NativeCompatible` is the unchanged strict-null default. New explicit
`OwnedRawIndexedFile` describes the original file selected by the real named
group's exact/lowercase indexes. Only fresh owned named inputs can use it;
observed native resources cannot. No listener is called or dereferenced.
Snapshots include full64-bit listener identity and semantics. Both are checked
before publication and for every receipt at bank commit. Source bytes still need
the existing deny-write/delete ancestor/file lease, exact size/hash and Main
revalidation. Unknown/modded content refuses rather than silently changing the
base-source contract.

Native game reads establish DDS basename+Landscape. PNG material textures use
AUTODETECT; this is not proof of historical native group selection. The chosen
owned file contract uses the actual image constructor's Overlaymaps+basename,
including exact `blendMap.png`. Canonical asset pins and logical names are
different identities. Independent evidence and scope review are recorded in
`research/gsm-raw-indexed-semantics-20261103/report.md`.

## Verification

Source-receipts suite:66 real lease/gate checks,182 SDK layout/string/lock/index
checks,41 listener checks under both/O2 and/Od,29 Win32 fault cutpoints, nine
existing and four listener/semantics mutation controls. A listener-replacement
regression was observed failing before snapshot comparison was fixed.

The full private probe builds successfully. Installed raw-indexed candidate is
687616bytes, SHA256
`74ac14c020e7bdd120235a98b26320f40e2697f7e1a9f894b9c9f7bec321c65c`.
The default/opt-in caller audit was independently reviewed before installation.
The changed provider received separate static review; no blocking defect found.
WorldStream was rebuilt against the enlarged Main-only snapshot:12 lifecycle
checks each/Od and/O2,268 polling-dependent flow checks, three completed WARP
frames, exact frozen material/normal/radial controls, OFF/world cancellation.

The compiled standalone native contract DLL remains unexecuted. These host tests
do not invoke Ogre. Complete native frame integration, image acceptance,
throughput, FPS and long-running memory measurements remain separate work.

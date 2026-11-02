# Immediate resident-window coverage — 2026-11-02

User accepted relative zoom and MCM layout, then reported native-map strips at
left/bottom during fast zoom-out. Production-extracted tests reproduced both
bands with all16 fallback textures already resident/shaded: old fastpath resized
widgets but left newly exposed ones hidden/unbound until200ms Draw. Pan-only
changes were skipped entirely by the old size-only stamp.

PresentWindows now shares final Draw presentation. RepositionZoom refreshes it
on native image size or relative viewport/clip change before streaming/busy
returns. Newly shown residents bind their existing texture as well as position,
visibility and original mip depth. Unchanged frames do no projection/iteration
or UI mutation. Completion/shade adoption still presents through Draw. Clear,
new layer and foreign parent invalidate the stamp; no new pointers/resources.
No source reads, requests, uploads or relights were added to this path.

Fresh author suite16checks/6compiledmutants and independent17/8 controls pass;
pre-fix source fails the intended actual functional test. Independent fullviewer
and compatibilityentry exit0. Root VC10/LTCG build+80imports exit0. Review:
research/gsm-presentation-review-20261102/report.md. Fake-widget benchmark median
0.0793ms per changing view with144 resident slots; not a game FPS measurement.

This fixes ready-resident stale presentation, not a genuinely never-loaded or
unshaded fallback. New coarse masks within existingDebug5s stats distinguish
valid/shaded/bound/visible slots for live diagnosis. Source prefetch policy is
unchanged. Live acceptance remains separate. No public release/version/journal.

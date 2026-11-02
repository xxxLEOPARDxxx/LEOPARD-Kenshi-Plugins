# Relative global-map input and MCM layout — 2026-11-02

Development change only; no public release or release-journal entry.

At extended maximum zoom, native fixed-size input took too many actions.
The production wrappers keep the original native cursor/centre anchors, using
returned KenshiLib trampolines exactly once. Button step is max(native minimum,
0.2*current); a stack scope restores native minimum before returning. Wheel
input keeps raw sign and magnitude, with gain max(1,current/(4*minimum)); the
accelerated integer saturates before conversion. No fixed wheel-notch size is
assumed. Native minimum remains reachable with Out. Reentry is bypassed.

Activation requires all three hook results/trampolines, owned cap lease, enabled
satellite view and multiplier>1. Foreign cap changes suspend both extension and
acceleration. Required hooks must initialize before optional input hooks.
Sixteen exact24-byte native windows are checked once before own hook patches,
including button/wheel entries and both minimum tails. A cap reduction invokes
the saved original In directly, avoiding wrapper recursion/acceleration.

Fresh root checks: full run_viewer_tests.ps1 exit0; step policy157706 assertions;
actual wrapper73 assertions+11 compiled failing mutants; existing caller28+8,
maximum policy132+6, plus existing viewer/water/model contracts. Full native
VC10/LTCG build exit0;80 native imports and MCM export checked. Binary:
786432B, SHA256 E2E2C1E90A527505680D74E7C02D7EB1993AB08C33E7463E820D6240D1F9B19E.
Independent review also requires the generated header snapshot, tracked in
research/gsm-zoom-step-review-20261102.

No new input-path heap allocation, retained map pointer, thread, cache or INI
poll. Wrappers do bounded scalar work, stack guards and existing WidgetRef
lookup. C++ exception restoration is covered; arbitrary SEH faults/external
hook systems and live responsiveness/FPS/long-memory behavior are not proved
by these host tests. Extra zoom cannot recover detail beyond the captured data.

The red disk warning now uses the native row's proportional3% left inset and
moves down to10px (bounded for small hosts), reusing its existing child widget.
MCM list outer width centres the native scrollbar gutter between button frames
and divider. Separate actual-BuildTab suite19192+4mutants and independent native
client/skin review cover vanilla/Dark geometry; neither substitutes live visual
acceptance. No public MCM version bump/preparation is implied here.

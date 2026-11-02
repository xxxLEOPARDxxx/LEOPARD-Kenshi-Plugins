# Optional global-map maximum zoom

Development setting ZoomMultiplier1..8, default1; applied only while satellite
view is enabled. It increases the native maximum without replacing wheel/button
input, marker thresholds, map projection or minimap sources. The original maximum
is restored when disabled; reductions below the current zoom call the exact
audited native zoom-in method once to clamp and rebuild the centre anchor.

Native Steam proof: zoom-in48BB50, zoom-out48BC90, updateZoom4907E0. SDK comments
name the GOG addresses and must not be used as Steam addresses. Constructor sets
maximum1.5*UIscale and highest marker threshold.75*UIscale; hence the baseline is
exactly2*zoomLevels[2]. MapScreen::clear resets current to minimum, preserves max,
and resizes the widget later. The extension never compounds the overridden max
on load; it defers while native zoomUpdated is pending. WidgetRef identity guards
lease lifetime; a foreign maximum writer suspends our override. Failure rolls
back maximum/current/anchor/dirty state and the previous lease.

Profile validation is cached and uses13exact native24-byte windows. DefaultOFF
has no map/native queries; unchanged frames do not write fields or resize widgets.
Target canvas is checked below1million pixels per dimension. No new GPU resources,
capture, filesystem polling or INI reads per frame are introduced by zoom.

Actual policy132checks and6compiledmutants PASS. Independent extracted caller28
checks and8compiledmutants PASS, including pending resize, repeated load, owner
destruction, foreign writer, profile failure and clamp exception rollback.
Full run_viewer_tests.ps1,22strings in3MO catalogs,8settings/Debuglast, actual PE
80imports and startup/MCM exports PASS. Native and caller reports retained under
D:/DEV/Kenshi/research/gsm-zoom-audit-20261102 and gsm-zoom-caller-review-20261102.

Installed784384byte DLL SHAe1e85bab03a4372404f8b0a311f6547634563c0bc39edfe88ea1c2f115ecea3e,
Main8f509381e8cb16516e01f3ab27625b88fa5fd066a85c53a7a744b3edb8313138.
Game was already closed. Backup before-maximum-zoom stores previous DLL/INI.
Only QA ZoomMultiplier=2 was added; other personal settings retained.

Prior live native max2 produced16384-square map/4096primary at UIscale1.33333.
Multiplier2 should produce32768-square map/8192primary, exposing existing source0
detail. Greater zoom enlarges captured terrain pixels; it cannot invent finer
terrain. Actual input/marker/anchor and visual acceptance after install are pending.
This development module is not published; no personal release-journal entry.

# Upstream provenance and integration boundary

AI Fixed combines independently authored LEOPARD fixes with selected ideas and
adaptations from Basusu's GateFix and SleepFix. Thank you, Basusu, for publishing
the originals and their investigation notes. RE_Kenshi/KenshiLib are by
BFrizzleFoShizzle; the shared localization loader is by Asur and contributors.

Original repository / Исходники оригинала:
https://github.com/Basusu0/Basusu-kenshi-plugins

Pinned upstream commit: `edcd54f89002b69c702733f256bf8d1677f51f39`.
Read-only detached research checkout:
`D:/DEV/Kenshi/research/Basusu-kenshi-plugins-upstream-20261101`.

| Original file | SHA256 |
| --- | --- |
| GateFix.cpp | `823a0aae995d313fe749f781c593dc341692709d4cb050082348c1d96584ad57` |
| SleepFix.cpp | `75178c35e750cb7f35361b06e0f27c9a9f9034ca0207cd488aa9d59cd5c47cdd` |
| LICENSE | `230184f60bae2feaf244f10a8bac053c8ff33a183bcc365b4d8b876d2b7f4809` |

The GPLv3 license was copied verbatim to LICENSE before implementation. No
CorpseLoot or FurnaceSlots functionality is included. The upstream source is
retained unchanged in the research checkout; AI Fixed uses the actual KenshiLib
headers/profile and does not copy upstream's guessed class layouts or offsets.

The original README's Workshop links are placeholders (`#`). The real public
Workshop pages were independently resolved and all public comments read (47
SleepFix / 11 GateFix at audit time):
- SleepFix: https://steamcommunity.com/sharedfiles/filedetails/?id=3787375792
- GateFix: https://steamcommunity.com/sharedfiles/filedetails/?id=3787399736
The pinned GitHub code and the public Steam binary are not assumed identical.
Issue #1 was read: https://github.com/Basusu0/Basusu-kenshi-plugins/issues/1
It reports fully healthy workers sleeping and sometimes starving. Steam report
URLs and access results are recorded in the integration verification report.

## Design and regression scope

One GUI-owned rest scheduler handles biological sleep, robotic health repair and
the existing wear repair. A single serial-bearing identity/reservation system
prevents competing bed orders. Search is bounded by configurable radius and bed
function, ownership, floor, free slot and repair power. New modules are off by
default pending gameplay acceptance. Debug remains last and off by default.

Automatic gate passage is separate from Gate Push Guard's anti-push correction.
It is driven by bounded player movement intents, waits for the native click storm
to stop and for an actually stalled actor, then uses ordinary door orders. No
forced-close/teleport fallback is imported. Original hook chains must run once.

Manual orders always take precedence. Completion must match an owned rest episode
and its exact target; global `clearOrders()` is not used to resume jobs. Load/new
world invalidates all transient state. No permanent AI job is removed or toggled.

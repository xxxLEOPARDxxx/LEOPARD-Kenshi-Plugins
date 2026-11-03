# Progressive viewer / medium-zoom fallback, 2026-11-03

Installed candidate:66364c4b471b14de66e41a05a342fcf744e060f9be534f1af40dc787607dabb5,1,095,680 bytes. Existing1906 atlas unchanged; all four personal INI hashes unchanged in the bundled install receipt at D:/DEV/Kenshi/backup/open-grades-map-keo-20261103/install-receipt.json.

The first live progressive candidate ef9bc8d... was confirmed by the user to remove ordinary-map strips during pan/zoom. The user then left zoomx2.25 blurry. Actual log:primary9207,viewport2160x1044,density8.00782; fine state spacing9,pairs0/0,GPU0,ordinarysource5/detailReady0,resident16. A3m visible set requires288slots versus96 allowed. Density-only admission suppressed ordinary refinement even though the fine planner deliberately refused this view.

The new candidate uses actual spatial feasibility. Infeasible fine views restore ordinary128-detail-slot capacity. Feasible startup retains2 ordinary details while shrinking toward the existing32MiB production-view allowance; actual measured baseline28,637,998+2*1,310,720=31,259,438 bytes. Owned fine/source banks retain their actual budget until drained. All16 overview fallback slots stay available. Same-stamp overlapping fine tiles survive pan; completed pairs can publish after the real source drain rather than waiting for the entire batch. Physical retirement debt stays accounted; routine retirement is limited to2slots/update. Existing192MiB aggregate gate is preserved.

HUD now reports actual zoom and simple loading/loaded status using full visible coverage rather than center-only readiness or an ambiguous tile counter. Three catalogues compile/read back. Old technical source density/center steps remain diagnostic data rather than player-facing labels.

Validation:34,600 fine-plan controls per/Od and/O2;814 WARP/viewer controls per/Od and/O2; extracted production pair completion with old-batch mutant rejected; actual coordinator430controls; final spatial-feasibility7,extracted capacity261,HUD13 controls per/Od and/O2. Independent review CLEAR. Linked native build verified startup/MCM exports and85 current game-library imports. See tests/fine-feasibility/candidate-report.json and the focused verification records.

Awaiting live user confirmation of medium zoom after this installation. These bounded controls do not prove all leaks/FPS behavior or complete city rendering. Current city geometry is still primarily distant proxy data; complete loaded walls/stalls/lamppost composition is separate unfinished work. No new atlas rerender, public version or publication was performed.

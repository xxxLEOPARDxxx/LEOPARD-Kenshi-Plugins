# Taunt startup and repeated DF earned-save load — 2026-10-31

## Taunt Expansion v1 candidate

Installed DLL SHA-256: `b16d292185d93c02e64edd030f16c1247686faf82b80e7b5f8eb896e6b727636`.
The user restarted Kenshi, loaded the earned DF scene, opened Taunt Expansion in MCM and closed the menu.
The new log contains module loading, Russian locale loading and repeated Taunt page updates; it does not contain the previous initialization error or MCM apply crash.
A bounded read-only process check (73 bytes, exact executable/DLL hashes and creation time) confirms the Taunt-specific instruction branches to the expected code cave, returns to the original continuation and leaves the shared vanilla +2 constant unchanged. The target-selection entry transfers through the loader trampoline into this exact Taunt DLL.

This confirms actual installation of the gameplay hook, beyond the mere presence of an MCM export. It does not establish combat balance, the result of changing a setting, UI layout on every skin, CPU/FPS cost or long-term memory stability. Debug remained off; this visit did not intentionally change personal settings.

Local evidence: `game/_codex_work/release-candidates-20261031/taunt-live-20261031-2349/` (archived RE log and native-startup-proof.json). No native game calls, process writes, injections or remote threads were used by this check.

## Dynamic Factions C9: repeated earned-save load

Two independent loads of the same completed `DF-care-earned-20261031` save both produce six observed members, zero unloaded platoons and exactly 3348 cats. All 46 physical inventory records match as a semantic multiset by owning actor, prototype SID, stack quantity, full and remaining charge bits, equipped status and proper owner. The bought backpack remains equipped by Sanka. Raw runtime item handles differ across processes and are deliberately not treated as saved identity.

This is evidence against loss/duplication when repeatedly loading this completed earned save. It is not an atomic pre-save versus post-load proof, an interrupted-sale/reentrant-save test, an unloaded-sector test or a skeleton-supply/performance test. Those acceptance boundaries remain open.

Local evidence: `game/_codex_work/dynamic-factions-20261031/care-acceptance/C9-repeat-load-comparison.json` and its two referenced guarded snapshots. The preliminary raw-handle comparison is retained separately and is not a semantic loss finding.

## Русский

Taunt Expansion теперь действительно установил перехват выбора цели и отдельное усиление провокации. Страница MCM открывается и закрывается без прежних ошибок. Изменение значения настройки, бой, FPS и длительный рост памяти этой проверкой ещё не подтверждены.

Две загрузки одного завершённого сохранения DF дали одинаковые 3348 ката, шесть участников и 46 записей предметов с одинаковыми количествами, остатками ресурса, владельцами и состоянием экипировки. Рюкзак сохранён. Проверка сохранения прямо во время сделки и другие граничные случаи остаются отдельными задачами. DF не публикуется.

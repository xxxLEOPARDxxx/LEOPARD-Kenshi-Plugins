# Dynamic Factions: medical supplies, backpack purchase and operating reserve

**Goal:** Real first-aid/repair supplies and suitable initial backpacks, bought
from real merchant stock with faction funds. Preserve the proven C8 food cycle.

**Architecture:** Extend the existing bounded self-supply route and paid-first
transaction. Native inventory, equip, physiology and medicine stay authoritative.
Pure policy ranks needs/bags and computes reserve; adapters read actual stock.
No second wallet, generated goods, player trading context or persistent pointers.

**Execution:** Inline, using executing-plans. User has opted out of skill-only
approval gates. Existing clean checkout is reused; independent review before
installation. DF remains unpublished.

## 1. Native contracts and pure policy

- [x] Verify installed SDK exports/Steam methods for bag retail value, race
  eligibility and native equip. Read code only from a verified game process.
- [x] Add pure medical need and bag suitability/ranking policy in DfSupply:
  organics need first aid; robots need repair kits; biological prostheses need
  both. Stock is real remaining resource, food alone uses NU*0.01.
- [x] Food survival first, medical stock next, optional backpacks last. Desired
  stock is bounded; acquired stock removes its deficit instead of reserving twice.
- [x] Tests cover robots/prostheses, partial stacks, invalid values, low funds,
  light-runner penalties, useful capacity, worse/equal bags and unknown quotes.

## 2. Read-only stock and market quotes

- [x] Extend bounded InventoryView reads to one combined stock snapshot. Include
  worn bags through physical sections and avoid double counting borrowed items.
- [x] Publish bounded shop quotes from the same persistent native stock as C8:
  full clean food, first aid, repair kits and empty backpacks. Commodity flag is
  not a sale gate. Inspect at most one shop per economic step, cache handles/SIDs,
  amounts/prices and bag metrics only; expire/reset quotes on world changes.
- [x] Reserve calculation uses actual remaining deficits and native edibility.
  Protect recruitment commitments plus an operating buffer (at least1000cats,
  scaling with current membership). Unknown/stale/incomplete forecasts forbid
  gear purchases. Bound forecast time and fail closed instead of extending a
  large inventory/world traversal across the frame.

## 3. Shared physical purchase and native equip

- [x] Generalize food offer selection without changing its price or transaction
  order. Revalidate actual stock, race, funds, price and space on arrival.
- [x] Medicine can merge through native addQuantity; use paid-first settlement
  and never extract a partly used item for compensation.
- [x] Quote/equip actual ContainerItem metrics: area/stacking, weight reduction,
  empty weight, athletics/combat-speed/skill penalties and race restrictions.
  Prefer useful low-penalty value over prestige. Use an already owned bag first.
- [x] Buy only when the native backpack slot is available. Purchase and equip
  are separate confirmations; an owned unworn bag prevents repeat purchases
  across ticks/reload. Preserve old bags and contents; replacing occupied bags
  belongs to the later gear-upgrade stage, not an unsafe swap here.
- [x] Native equip refusal leaves the paid item in real inventory. Ambiguous
  transfer/equip faults quarantine, never replay payment or discard items.

## 4. Integrate the single supply route

- [x] DfProvision stores supply kind alongside full member/trader handles.
  Choose medicine/bag routes only after urgent food; preserve native medical,
  rescue, sleep and combat preemption. Work=0 still allows necessary supplies.
- [x] Reuse route timeouts/cooldowns, one transaction per5-second step and
  existing receipts/events. Add RU/EN/CN user messages and reasoned Debug logs;
  debug stays last/default0. No extra frame hooks or background native access.
- [x] Save/load derives stock/equip state from actual items, resets ephemeral
  quotes/routes and does not recreate a purchase from an old UI event.

## 5. Verification and deployment

- [x] Production-extracted adapters, core edge tests, startup binding, import
  audit, independent correctness/perf/lifetime review and full compatible build.
- [x] Install with game closed, preserve personal INI/.mod and C8 backup.
- [ ] Live physical medicine/bag purchase: seller goods decrease, buyer goods
  increase, both wallets reconcile; native equip/consumption behave correctly.
- [ ] Separate earned save/reload preserves money, charges, worn bag and cargo.
- [ ] Measure native quote/stock traversal cost and long memory separately;
  tests/review alone do not prove the absence of leaks or an FPS cost.
- [x] Bilingual README and honest evidence/remaining limits.
- [ ] Meaningful commit/push of the verified local implementation.

Implementation decisions: quotes are learned only on real visits; all physical
backing-pool identities are registered before capped offers. Pool aliases are
invalidated together, and finite remaining units are allocated across members.
Owned nested/unwearable bags block repeat buying; occupied-bag swaps are deferred.
The2ms forecast budget is checked between shops/members and cannot interrupt an
individual native call. Review findings and fixes are documented in README C9.

Build/install evidence: DLL409088bytes, SHA256
b06c31607199a92b873c1b35baaf840d820aae8bca04f385d579f654c2b8aa6d.
1276+40+37+35+23+20+39=1470checks passed, plus session source contracts.
132native imports and64source strings in each of3MO catalogs verified.
C8DLL and replaced files backed up under game/_codex_work/
dynamic-factions-20261031/care-acceptance/C8-before-care.
INI/.mod/defaults/loader config unchanged. DF-care-20261031 is an exact
85-file SHA-verified copy of DF-food-earned-20261031; source untouched.
Live acceptance is pending the user's next scene load, not assumed from tests.

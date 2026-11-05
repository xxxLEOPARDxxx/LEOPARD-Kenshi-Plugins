# Carried weight implementation plan

> For agentic workers: execute this scoped plan task by task; parent independently reviews before installation.

Goal: port missing carried-body and nested-container weight accounting while preserving native weighing.
Architecture: one Inventory::getTotalWeight adapter, pure bounded calculation over native callbacks, stack TLS baseline capture. Default OFF; exact SDK parity and no loaded KenshiQOL required.
Tech: VC10 C++, actual production host fixtures, existing KEO config/MCM.
Spec: weight-audit.md and parent scoped authorization.

Constraints: no install/commit/publication; no other jobs/navmesh changes. No native body constant, per-call heap allocation or world scan; preserve worn-bag reduction. Startup-only switch reports restart honestly.
Review focus: worn/loose distinction; malformed/cyclic graphs; carried chains; exception/thread TLS restoration; late startup QoL conflict.

- [x] Write meaningful missing-feature RED from owned native weight facts; compile and observe expected failure.
- [x] Add carried_weight_policy.h/.cpp bounded arithmetic/graph traversal and tests including native body delta, hunger, robots, scale, alias/cycle/cap and nonfinite inputs.
- [x] Add carried_weight.h/.cpp adapter and exact SDK/native contract checks; TLS original once per invocation, main1Hz admission, no per-call module lookup. Extract actual hook in tests for nested/unwind/concurrency.
- [x] Add startup-only config default OFF, MCM layout row, EN/RU/ZH text and provenance/README. Add build TU lists and tiny existing Main tick call.
- [x] Run focused tests plus relevant config/layout checks. Build DEV/PROD incrementally using validated unchanged object sets. Send hashes/limitations/files to parent; do not install or commit.

Build: existing standard noenv runner, DEV/PROD succeeded148 KenshiLib imports each. Cache mode on but observed0hits/345compiles each; reused the same candidate folders, no duplicate broad fixture outputs. No install/commit/publication by build. Independent macro review CLEAR. Final verification: leopard/carried_weight/verification.json.

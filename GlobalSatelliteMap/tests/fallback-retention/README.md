# Persistent overview coverage regression

The real fine viewer deleted every offscreen fixed overview tile at close zoom. After a fast pan or zoom-out those already-warm tiles had to be read, uploaded and relit again, exposing the underlying ordinary map. A later fine-material refusal did not restore the deleted coverage immediately.

The fix removes only that eviction and gives fine-qualified mode zero additional base-detail slots. All sixteen overview slots remain persistent until ordinary session/world cleanup; normal or failed-fine mode retains its previous 128 detail capacity. Fine startup/production already charge complete requested layers, retained center and pending GPU retirement, so insufficient allowance defers fine work rather than deleting overview coverage. No geometry, projection or material validation changed.

`run.py` extracts the actual production residency-maintenance tail and BaseDetailCapacity into a minimal host harness, compiled VC10 /Od and /O2. It uses the real AtlasWindowPolicy planner for 128 pan/zoom layouts. Initial RED is retained in red-fallback-eviction.log. GREEN confirms all sixteen shaded fallback keys stay ready, only detail slots retire, and blocked fine retains normal detail capacity. No native/game/GPU execution, disk fixtures or long-run performance claims.

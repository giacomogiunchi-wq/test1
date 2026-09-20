# Milestone 5.1 implementation checklist

## M5.1A — Instrumentation and corpus

- [x] Thread-safe performance recorder and RAII spans.
- [x] Deterministic median/p95/p99/max summaries.
- [x] Frame, asset-load, and assembly-interaction profiler records.
- [x] Chrome Trace JSON export with escaping and stable ordering.
- [x] Deterministic LA-01 through LA-07 corpus specifications and codec.
- [x] Baseline, bottleneck, and architecture-risk reports.
- [x] Tests and instrumentation/corpus benchmark.

## M5.1B — Definition/revision/occurrence runtime
- [x] Immutable definitions/revisions, occurrence graph, hierarchy, and invalidation domains.
- [x] Strong hash roles, body revision identities, copy-on-write contracts, and M5.2 command integration.
- [x] Repeated-occurrence tests and 10k/50k runtime benchmark.

## M5.1C — Container and local cache
- [ ] Chunked authoritative prototype, asset cache, SQLite catalog, raw/zstd decision data.

## M5.1D — Residency and progressive open
- [ ] Scheduler, budgets, FIRST_USEFUL/INTERACTION_READY, prewarming.

## M5.1E — Shared surfaces/edges/LOD
- [ ] Display assets and OCCT connected-presentation benchmark; renderer decision gate.

## M5.1F — Spatial index and selection
- [ ] Occurrence BVH and contextual detailed selection.

## M5.1G — Interactive paths
- [ ] Insert, transform, visibility, tabs, and edit-in-context invalidation assertions.

## M5.1H — Conditional instanced renderer
- [ ] Implement only if the renderer decision gate justifies it.

## M5.1I — Stress and network/cache hardening
- [ ] Cancellation, stale tasks, corruption, latency, and pressure tests.

## M5.1J — Regression gates
- [ ] Reference hardware tiers and validated performance envelopes.

Stop after M5.1B. No storage/cache, residency, rendering, or later runtime phase is claimed.

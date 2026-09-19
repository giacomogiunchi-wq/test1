# M5.1 architecture risks

| Risk | Consequence | M5.1 response |
|---|---|---|
| Treating an assembly as one authoring document | Global loads and recomputes | Separate domains in ADR 0046; implement in M5.1B |
| Coarse file/timestamp cache keys | Unrelated cache invalidation | Granular immutable hashes in ADRs 0048–0053 |
| Authoritative SQLite/WAL on NAS | Unsafe shared-memory/locking assumptions | SQLite is local and disposable only |
| Background stale result commit | New edits overwritten | Revision-token rule in scheduler ADR; test in M5.1D/I |
| Assuming connected AIS equals GPU instancing | Excess CPU draw submission | Benchmark first; renderer decision deferred to M5.1E |
| Instrumentation perturbing interaction | Misleading tail measurements | Measure recorder overhead; add bounded/per-thread strategy only with evidence |
| Proxy used for engineering answers | Incorrect measurement/selection | Residency and representation capabilities remain explicit |
| Premature marketing thresholds | Optimizing noise or one machine | Preserve distributions and publish median/p95/p99/max |

The largest immediate risk is mistaking M5.1A instrumentation for a large-assembly implementation. Phases B–J remain incomplete.

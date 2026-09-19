# M5.1A instrumentation and benchmark corpus

```text
call site ── PerfSpan / typed sample ──> Duomec runtime instrumentation
                                            │
                                            ├── immutable event snapshot
                                            ├── percentile summary
                                            └── Chrome Trace JSON

deterministic LA specification ── encode/decode ──> later benchmark generators
                                                        │
                       M5.1B occurrence runtime <────────┤
                       M5.1E render benchmark <──────────┤
                       M5.1G interaction paths <─────────┘
```

The instrumentation library depends only on `Duomec::Core`. Domain and adapter targets do not depend on it, and it contains no Qt, AIS, OCCT, SQLite, zstd, scheduler, assembly, or renderer types.

`PerfSpan` measures scoped durations. The recorder assigns sequence numbers while holding the event mutex, so snapshots are owned and safe across producer threads. The exporter orders by sequence and uses Chrome's complete-event (`X`) form. Typed frame, asset-load and interaction profilers retain the measurements called for by later phases without claiming those systems exist today.

The corpus is versioned canonical text with fixed seeds. LA-01 has separate 10k and 50k variants; LA-02–LA-07 encode unique-part, production-mix, heavy-geometry, deep-tree, multitab, and network-source workload dimensions.

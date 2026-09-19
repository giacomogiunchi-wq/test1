# M2A framework microbenchmark baseline

This is a framework-overhead baseline, not a geometry or product-performance claim.

## Method

* Build: GCC 13.3.0, C++20, default CMake build type in the supplied Linux runner.
* Workload: 10,000 fresh sessions; initialize one scalar schema and one collector, consume one stable face-like reference, validate, and execute a deterministic preview adapter.
* Integrity: expected checksum is 20,000 and the process fails if it differs.
* Timing: one `steady_clock` interval around each complete loop; no file or geometry work. Two ordinary-build observations were recorded.

## Recorded baseline — 2026-09-19

| Workload | Iterations | Total | Mean/iteration | Checksum |
|---|---:|---:|---:|---:|
| `feature_framework_preview` | 10,000 | 159,774,702–167,327,836 ns | 15,977–16,732 ns | 20,000 |

The executable prints CSV so dedicated benchmark jobs can collect repeated samples. These two observations are not a regression threshold; ADR 0029 requires repeated samples and machine metadata before gates are introduced.

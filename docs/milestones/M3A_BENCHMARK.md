# M3A performance baseline

These numbers measure domain overhead only and are not OCCT geometry performance claims.

## Method

* GCC 13.3.0, C++20, default CMake build type, supplied Linux runner.
* `interpolated_law`: one million evaluations of a five-station monotonic cubic law across 1,001 repeating normalized parameters.
* `continuity_65_samples`: 1,000 G2 evaluations of two analytic straight boundaries, each using 65 samples.
* A shared non-zero checksum prevents dead-code elimination; each continuity result must pass. Two ordinary-build observations were recorded.

## Observation — 2026-09-19

| Workload | Iterations | Total | Mean/iteration | Checksum after suite |
|---|---:|---:|---:|---:|
| Interpolated law | 1,000,000 | 321,380,084–326,052,943 ns | 321–326 ns | 13,439.2 |
| Continuity, 65 samples | 1,000 | 54,963,036–55,812,754 ns | 54,963–55,812 ns | 13,439.2 |

These observations establish executable output, not a CI threshold. Optimized release builds, repeated samples, CPU metadata, and OCCT-backed cases are required before performance gates are set.

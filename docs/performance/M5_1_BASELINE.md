# M5.1A large-assembly baseline

## Scope and hardware

Measured on the dependency-free CI container on 2026-09-19. The container exposes Linux x86-64, but no Qt 6, OCCT graphics context, dedicated GPU telemetry, or representative NAS. Consequently these numbers are an **instrumentation baseline**, not a claim about assembly rendering.

The deterministic corpus describes LA-01 at 10k and 50k occurrences plus LA-02 through LA-07. It is deliberately a compact workload manifest: the definition/occurrence model and generated geometry belong to M5.1B and later.

## Results

| Operation | Samples | median | p95 | p99 | max |
|---|---:|---:|---:|---:|---:|
| Encode eight workload specifications | 1,000 | 3,579 ns | 3,808 ns | 4,117 ns | 287,101 ns |
| Decode eight workload specifications | 1,000 | 24,485 ns | 25,036 ns | 39,696 ns | 126,291 ns |
| Record one synchronized trace event | 10,000 | 703 ns | 775 ns | 15,204 ns | 5,191,465 ns |

The 10,000-event Chrome Trace JSON payload was 1,020,017 bytes. Results are observations from one run and are not regression budgets.

## Currently unmeasurable

Time-to-first-useful, interaction-ready, draw calls, GPU time/VRAM, OCCT connected presentations, edge rendering, cache hit rates, exact-geometry residency, tab switching, and edit-in-context have no implementation in the current repository. Reporting fabricated zeroes would hide the principal architecture gap.

## Reproduction

```sh
cmake -S . -B build -DDUOMEC_BUILD_DESKTOP=OFF -DDUOMEC_ENABLE_OCAF=OFF -DDUOMEC_BUILD_TESTS=ON
cmake --build build --parallel
./build/duomec_large_assembly_baseline
```

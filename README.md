# Duomec Platform

Duomec is an open-source, modular mechanical CAD/CAE application. This repository contains the Milestone 0 shell, the Milestone 1A document foundation, and the dependency-free **Milestone 2A unified feature interaction framework**, and the **Milestone 3A geometry-law and quality-measurement foundation**, and the **Milestone 4A discrete mesh/scan foundation**. The repository does not yet contain the assumed Milestone 1 sketch, recompute, topological-resolution, or solid-feature implementations; that prerequisite gap is documented rather than hidden.

## Build the CI-friendly core

```bash
cmake -S . -B build -DDUOMEC_BUILD_DESKTOP=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/duomec_feature_framework_benchmark
./build/duomec_geometry_quality_benchmark
./build/duomec_discrete_geometry_benchmark
./build/duomec_large_assembly_baseline
```

This dependency-light configuration exercises the domain transaction contract,
including 20-step undo/redo and semantic save/reload through the in-memory test
store. It does not masquerade as an OCAF integration test.

Large-assembly development currently stops at M5.1A: reusable instrumentation,
Chrome Trace export, and deterministic LA-01–LA-07 workload specifications. It
does not yet contain an assembly runtime or make renderer-performance claims.

M5.2 Step 5.2.1 adds only the solver-neutral assembly-relation metadata
foundation: lightweight absolute references, independent mobility/solve modes,
DOF reporting, compact geometry and local frames, future motion/FEM hints, and
versioned persistence. It does not add mates, a solver, or assembly UI.

## Build the OCAF Milestone 1A integration

With OCCT 8.x installed at the pinned integration baseline:

```bash
cmake -S . -B build-ocaf -DDUOMEC_ENABLE_OCAF=ON
cmake --build build-ocaf --parallel
ctest --test-dir build-ocaf --output-on-failure
```

The additional integration test creates a body and generic feature, persists
their UUIDs and SI parameter to a binary `.duomec` file, reopens it, and checks
OCAF-backed undo/redo.

## Build the desktop acceptance slice

Install the pinned Qt and OCCT development packages described in `DEPENDENCIES.md`, then:

```bash
cmake -S . -B build-desktop -DDUOMEC_BUILD_DESKTOP=ON
cmake --build build-desktop --parallel
./build-desktop/duomec
```

The viewport displays an OCCT box. Drag the middle button to pan, Shift+left-drag to orbit, use the wheel to zoom, and left-click a face or edge to display its topology type in the status bar.

## Design constraints

* C++20, SI units internally, deterministic numerical engines, and no owning raw pointers.
* Third-party engines exist only behind adapters; public interfaces use Duomec domain types.
* Later milestones are intentionally represented by empty package boundaries and disabled feature flags—not partial implementations.

See [`docs/milestones/M4_PREFLIGHT.md`](docs/milestones/M4_PREFLIGHT.md),
[`docs/architecture/m4a-discrete-geometry.md`](docs/architecture/m4a-discrete-geometry.md),
[`docs/licenses/THIRD_PARTY_GEOMETRY.md`](docs/licenses/THIRD_PARTY_GEOMETRY.md),
[`docs/milestones/M4_CHECKLIST.md`](docs/milestones/M4_CHECKLIST.md),
[`docs/milestones/M3_PREFLIGHT.md`](docs/milestones/M3_PREFLIGHT.md),
[`docs/architecture/m3a-geometry-quality.md`](docs/architecture/m3a-geometry-quality.md),
[`docs/milestones/M3_CHECKLIST.md`](docs/milestones/M3_CHECKLIST.md),
[`docs/milestones/M2_PREFLIGHT.md`](docs/milestones/M2_PREFLIGHT.md),
[`docs/architecture/m2a-feature-framework.md`](docs/architecture/m2a-feature-framework.md),
[`docs/architecture/m2-component-diagram.md`](docs/architecture/m2-component-diagram.md),
[`docs/milestones/M2_CHECKLIST.md`](docs/milestones/M2_CHECKLIST.md),
[`docs/architecture/m1-ocaf-schema.md`](docs/architecture/m1-ocaf-schema.md),
[`docs/architecture/m1-component-diagram.md`](docs/architecture/m1-component-diagram.md),
[`docs/milestones/M1_CHECKLIST.md`](docs/milestones/M1_CHECKLIST.md), and
[`docs/adr`](docs/adr).

Step 5.2.2 adds the solver abstraction, incremental relation islands, standard
relation semantics, descriptor-based Quick Mate controller state, and
transform-only manipulation. The dependency-free native backend currently
provides DOF/consistency classification rather than a general nonlinear
geometric solve; see `docs/milestones/M5_2_2_REPORT.md`.

Step 5.2.3 adds the metadata-only component lifecycle layer: default and
interactive insertion, replacement diagnostics, embedded virtual definitions,
atomic external extraction, hierarchy formation, copy-on-write independence,
per-occurrence mobility/solve mode, Mate References, and smart-insertion state.
It does not add advanced or mechanical mates.

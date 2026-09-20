# Milestone 5.2 Step 5.2.2 report

## Backend

- Backend: `NativeAssemblyConstraintSolver` 0.1, in-tree and covered by Duomec GPL-3.0.
- OndselSolver: not integrated. Network restrictions prevented verification of its official current source, version, license, build, incremental/warm-start API, closed-loop behavior, and diagnostics. Naming a fabricated adapter would be unsafe.
- Capability: solver-neutral interface, local-island request/result model, DOF accounting, fixed/under/fully constrained classification, and semantic duplicate redundancy/conflict detection. This is not yet a general nonlinear pose solver.

## Implemented behavior

- Incremental relation graph with local island merge and affected-island-only split.
- Coincident, Concentric, Distance, Angle, Parallel, Perpendicular, Tangent, Lock, Ground/Fix, Frame, and semantic Insert records.
- Concentric alignment/anti-alignment and explicit rotation lock.
- Signed angle plus persisted alignment/reference vector to prevent `acos(dot())` branch loss.
- Assembly-relation persistence advances to schema 2 while retaining schema 1 read compatibility.
- Descriptor-only Quick Mate candidate filtering, flip state, candidate highlighting, cancellation, and inline Distance/Angle values.
- Transform-only translation, right-drag rotation threshold/context-menu distinction, transient multi-selection grouping, explicit/local-origin/group-center pivot policy, and remaining-DOF projection.
- Island-local interactive/final drag calls with small interactive budgets, warm-start intent, stale-revision rejection, one history commit, and failure rollback.

## Benchmark

Dependency-free runner, 5,000 disconnected two-occurrence islands and 10,000 solves of one selected two-occurrence island:

| Metric | Result |
|---|---:|
| Graph construction | 1,405,995 µs |
| Solver input | 2 occurrences / 1 relation |
| Solve median | 8,311 ns |
| Solve p95 | 8,533 ns |
| Solve p99 | 30,772 ns |
| Solve max | 1,268,018 ns |

These are implementation baselines, not product guarantees. UUID generation dominates corpus construction and requires profiling before setting a gate.

## Known numerical and integration limitations

- The native backend classifies semantics/DOFs but does not numerically satisfy arbitrary geometric equations or closed loops.
- Right-drag currently applies a Z-axis virtual rotation from the supplied interaction angle; a camera-space full arcball mapping belongs in the viewport binding.
- Tangent and other standard semantics are persisted and accounted for, but general nonlinear geometry evaluation awaits a verified numeric backend.
- Warm-start fields and budgets cross the interface, but the native stateless backend has no factorization/Jacobian cache.
- M5.1B–J runtime, rendering, selection, and session prerequisites remain absent, so the overlay has no Qt presentation and transforms have no renderer attachment.

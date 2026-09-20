# Milestone 5.2 final report

## Architecture and scope

M5.2 provides domain-owned persistent relation semantics, occurrence-local transforms, solve islands, component lifecycle operations, advanced/mechanical relations, diagnostics, and explicit analysis compilation boundaries. Normal assembly operations depend on compact descriptors and local frames—not B-Rep, tessellation, AIS, Motion, or FEM objects.

## Relation coverage

| Family | Coverage |
|---|---|
| Standard | Coincident, Concentric, Distance, Angle, Parallel, Perpendicular, Tangent, Lock, Ground/Fix, Frame, Insert |
| Advanced | Limit Distance/Angle, Linear Coupler, Path, Profile Center, Symmetric, Width |
| Mechanical | Cam, Gear, Hinge, Rack and Pinion, Screw, Slot, Universal Joint, Belt/Chain |
| Diagnostics | Solved, Underconstrained, Redundant, Conflicting, Dangling, Suppressed, SolverFailed, NeedsReview |

## Solver backend, DOF, and closed loops

`NativeAssemblyConstraintSolver` 0.1 is in-tree and GPL-3.0 under Duomec. It classifies semantic constraints and six-DOF state, accepts interactive/final modes, revision stamps, budgets, and warm-start intent, and is invoked only for affected solve islands. It is not a nonlinear geometric solver. A four-hinge closed-loop graph is preserved as one island, but trajectory convergence is not claimed. OndselSolver remains unintegrated and unverified.

## Quick Mate and lifecycle results

Quick Mate filters compatible compact descriptors, supports alignment flip and inline Distance/Angle input, and creates persistent relation semantics without repeated topology traversal. Lifecycle operations retain occurrence identity and metadata: origin-aligned Fixed or interactive Floating insertion, Replace Component with explicit migration status, virtual definitions, external extraction, Form Subassembly, Make Independent with content-hash sharing, and per-occurrence Fix/Float and Rigid/Flexible state.

## Conflict and reference behavior

Structural duplicate and conflict detection distinguishes Redundant from Conflicting and returns the smallest pair it can prove. Repair attempts exact persistent resolution followed by descriptor/signature candidates. Ambiguity becomes NeedsReview and cannot silently select arbitrary geometry.

## Motion semantic mapping

Motion compilation is explicit through `IKinematicModelCompiler` and `IMultibodyRelationExporter`. It maps Ground, Lock, Hinge, Prismatic, Universal, Screw, Gear, RackPinion, Belt, Path, and generic semantics to neutral compiled entities. Stable local frames are validated and retained for future forces, moments, joint reactions, and plotting. Confirmed high-level joints supersede redundant low-level mates. The Project Chrono prototype table is in `docs/analysis/M5_2_PROJECT_CHRONO_MAPPING.md`; no Chrono object or time integration is created.

## FEM hint mapping

`IAssemblyToFemCompiler` and `FemRelationHintExtractor` map Coincident faces to Contact/Bonded candidates; Hinge/Concentric to Joint/Connector/Bearing candidates; Lock/Fixed to Rigid/Bonded candidates; and Frame references to Remote Point/Connector Frame candidates. Every candidate requires review and starts unconfirmed. Only scoped endpoints are resolved through the on-demand geometry port; no contacts, Code_Aster objects, meshes, or connectors exist beforehand.

## Analysis caches and architectural assertions

Motion and FEM cache templates are separate from assembly runtime caches and from each other. Their entries record explicit occurrence scopes; an unrelated occurrence change does not invalidate the compiled model. Automated tests prove that Motion compilation never invokes exact geometry, FEM resolves only in-scope endpoints, normal graph/solver/browser operations do not invoke the geometry resolver, unrelated solve islands remain separate, compiled objects do not exist before explicit compilation, and invalid reaction frames are rejected. The assembly relation target has no tessellation or feature-recompute dependency, so mate/drag/browser paths cannot call those systems.

## Hardening benchmark

Dependency-light runner; microbenchmarks report microseconds. Results are baselines, not product guarantees.

| Workflow | Median | p95 | p99 | Max |
|---|---:|---:|---:|---:|
| Many small islands | 6,838.55 | 8,473.42 | 8,473.42 | 8,473.42 |
| One 300-occurrence island | 172,411 | 174,905 | 174,905 | 174,905 |
| 5,000 relation diagnostics | 37,428.6 | 40,609.3 | 41,508.3 | 41,508.3 |
| Quick Mate candidates | 0.199 | 0.261 | 0.415 | 4.303 |
| Constrained-drag solve | 11.734 | 12.452 | 30.198 | 32.474 |
| Flexible mode update | 0.497 | 0.560 | 0.615 | 4.804 |
| Replace Component | 11.259 | 36.422 | 45.220 | 45.220 |
| Create Subassembly | 552.338 | 597.108 | 611.172 | 611.172 |
| Make Independent | 48.664 | 62.403 | 64.381 | 64.381 |
| Origin/plane mate graph update | 39.082 | 47.876 | 58.391 | 59.785 |
| Relation browser grouping | 240.835 | 262.682 | 264.770 | 264.770 |

The current graph rebuild algorithm is the dominant bottleneck for a single large island and needs an incremental connectivity structure before a production gate. The flexible-mode benchmark measures metadata transition only because flexible solve expansion requires a production numerical backend.

## Known limitations

- No production nonlinear solve, mechanism trajectory, physical cam contact, collision, or time integration.
- No actual Chrono or Code_Aster adapter; upstream APIs, licensing, packaging, and numerical behavior require a dedicated integration spike.
- FEM candidates are semantic hints, not validated physical boundary/contact conditions.
- Exact geometry resolution is an interface; production OCAF/cache integration is pending.
- M5.1B–J runtime, renderer, residency, and cache prerequisites described by earlier reports remain incomplete.
- Browser/glyph/Quick Mate domain state is not a finished Qt viewport experience.

## Motion handoff

Implement and validate a Project Chrono adapter behind the exporter/compiler interfaces; translate local frames and parameters; add units and sign-convention conformance tests; implement initial conditions, loads, time integration, reaction extraction, and result provenance; preserve scope-aware cache keys and never place Chrono objects in the normal assembly runtime.

## FEM handoff

Implement reviewed candidate editing, geometry-resolution integration, contact/connector policies, material and mesh assignment, Code_Aster model generation, solve execution, and result provenance. A user must explicitly confirm every physical condition; CAD mates must never silently become FEM conditions.

## Completion boundary

M5.2 stops at analysis readiness. Full multibody dynamics and FEM solving are not implemented.

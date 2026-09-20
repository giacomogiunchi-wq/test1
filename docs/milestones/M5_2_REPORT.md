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
| Many small islands | 2,771.07 | 4,726.25 | 4,726.25 | 4,726.25 |
| One 300-occurrence island | 4,683.31 | 5,054.97 | 5,054.97 | 5,054.97 |
| 5,000 relation diagnostics | 34,144.7 | 37,988.5 | 39,513.0 | 39,513.0 |
| Quick Mate candidates | 0.198 | 0.234 | 0.376 | 4.855 |
| Constrained-drag solve | 10.987 | 12.167 | 29.579 | 39.528 |
| Flexible mode update | 0.481 | 0.574 | 0.600 | 4.847 |
| Replace Component | 11.796 | 28.487 | 49.052 | 49.052 |
| Create Subassembly | 68.192 | 141.415 | 172.524 | 172.524 |
| Make Independent | 24.532 | 41.289 | 55.979 | 55.979 |
| Origin/plane mate graph update | 13.137 | 13.761 | 23.582 | 38.290 |
| Relation browser grouping | 241.536 | 268.823 | 287.175 | 287.175 |

Single-island insertion now updates the touched island in place; the 300-occurrence chain median improved from 172,411 µs to 4,683 µs. Multi-island merges and splits still rebuild affected connectivity and need larger production-corpus validation. The flexible-mode benchmark measures metadata transition only because flexible solve expansion requires a production numerical backend.

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

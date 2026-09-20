# Milestone 5.2 sequential checklist

## Step 5.2.1 — semantic foundation

- [x] Relation, endpoint, descriptor, local-frame, DOF, motion, and FEM-hint domain types.
- [x] Lightweight absolute references for part and assembly definitions.
- [x] Independent mobility and subassembly solve modes with all combinations tested.
- [x] Default origin-aligned Fixed insertion without fake mates.
- [x] Versioned persistence and regression tests.

## Step 5.2.2 — standard constraints and manipulation

- [x] Solver-owned interface and dependency-free baseline backend.
- [x] Incremental solve islands and island-local drag requests.
- [x] Standard/Insert relation semantics, Quick Mate state, manipulation, persistence, and history tests.

## Step 5.2.3 — component lifecycle and hierarchy

- [x] Default/interactive insertion, Fix/Float, and per-occurrence Rigid/Flexible.
- [x] Replacement with ordered reference migration and explicit outcomes.
- [x] Virtual components, atomic external save, Form Subassembly, and Make Independent.
- [x] Lightweight Mate References and smart-insertion preview/commit.
- [x] Lifecycle persistence, undo/redo, regression tests, and performance baseline.

## Step 5.2.4 — advanced/mechanical relations and diagnostics

- [x] Semantic advanced relations with limits, modes, stable frames, and path parameters.
- [x] Mechanical relation records with ratios, directions, phases, offsets, and motion intent.
- [x] Distinct diagnostics, localized known conflict sets, and explicit reference repair.
- [x] Metadata-only relation views/browser and confirmation-gated joint recognition.
- [x] Persistence, history, closed-loop graph, regression tests, and diagnostics benchmark.

## Step 5.2.5 — analysis readiness and hardening

- [x] Explicit Motion compiler/exporter interfaces and neutral semantic mapping.
- [x] Review-required FEM candidate extraction with scoped geometry resolution.
- [x] Separate scope-aware analysis caches and stable reaction frames.
- [x] Runtime architectural assertions and percentile hardening benchmarks.
- [x] Final M5.2 report and Motion/FEM handoff contracts.

## Deferred milestones

- [ ] Production nonlinear backend and viewport integrations.
- [ ] Full multibody dynamics and FEM model generation/solving.

Milestone 5.2 stops after Step 5.2.5.

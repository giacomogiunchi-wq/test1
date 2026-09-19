# Milestone 2 implementation checklist

## M2A — Unified interaction and execution framework (this change)

- [x] Domain-owned feature definition and progressive-disclosure parameter schema.
- [x] Shared selection rules/collectors with preselection consumption and stable references.
- [x] Shared body operation, extent, direction, diagnostics, and manipulator types.
- [x] Preview/final quality modes, cancellation, commit/cancel lifecycle, and edit context.
- [x] Executor and application-transaction ports; no Qt or kernel semantics in the framework.
- [x] Tests for schema validation, selection-first flow, preview, cancellation, single commit, and edit reuse.
- [x] Repeatable framework microbenchmark with checksummed output.

## M2B — Production solid features

- [ ] Restore/complete M1 feature prerequisites, then migrate every solid feature to M2A.
- [ ] Shared exact-geometry validation and extent semantics.

## M2C — Multibody and Save Bodies

- [ ] Solid/surface/mesh body collection, multi-result semantics, provenance, and exports.

## M2D — Round/chamfer framework

- [ ] Constant-radius multiple sets, propagation, isolated diagnostics, and regression corpus.

## M2E — Surface foundation

- [ ] Surface body features, boundary continuity intent, sew/trim/thicken operations.

## M2F — Mesh body foundation

- [ ] `IMeshKernel`, separate persistence, VTK adapter, import/cleanup/statistics.

## M2G — Storage and performance

- [ ] Authoring/geometry/tessellation/metadata/reference separation and large-model benchmarks.

Stop after M2A. Unchecked phases are architectural decisions only, not partial implementations.

# Milestone 1 implementation checklist

## 1A — OCAF foundation (this change)

- [x] Document schema and file-format version documented.
- [x] Strong persistent UUID types for document, body, feature, and parameter entities.
- [x] Authoritative document/body/generic-feature/parameter domain aggregate.
- [x] Storage port with transaction, undo/redo, atomic save, and load contracts.
- [x] OCAF standard-attribute adapter using `TDocStd_Application` and `BinDrivers`.
- [x] One command transaction per mutation with abort on failure.
- [x] Unit tests, 20-step undo/redo stress test, and optional binary OCAF integration test.

## 1B — Sketch and PlaneGCS

- [ ] Sketch-owned geometry and constraint variants, serialization, status, and DoF.
- [ ] Pinned PlaneGCS extraction and adapter; rectangle solve/edit/save/reload acceptance.

## 1C — Profile and Pad

- [ ] Validated 2D-to-3D profile pipeline, nested loops, Pad driver and recompute.

## 1D — Pocket, Revolve, Boolean

- [ ] Validated feature drivers and dependency relationships.

## 1E — References, Fillet, Chamfer

- [ ] TNaming/history plus signature/adjacency fallback and explicit ambiguity.

## 1F — Recompute and diagnostics

- [ ] DAG dirty propagation, failure blocking, stale visualization, and UI diagnostics.

## 1G — Hardening

- [ ] Golden models, deterministic metrics, save/load suite, leak checks, and M1 report.

Work must stop after 1A passes; unchecked sections are not partially implemented.

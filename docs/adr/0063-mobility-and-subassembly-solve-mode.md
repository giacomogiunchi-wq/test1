# ADR 0063: Orthogonal mobility and subassembly solve mode

* Status: Accepted
* Date: 2026-09-19

## Context

Placement relative to a parent and exposure of internal subassembly DOFs answer different questions.

## Decision

`PlacementMobility` (`Fixed`/`Floating`) and `SubassemblySolveMode` (`Rigid`/`Flexible`) are independent persisted values. Fixed is an occurrence boundary condition, not a generated fake mate. Flexible exposes only rigid internal component DOFs in a later solve phase; it never means flexible-body deformation.

## Consequences

All four combinations are representable without special cases, and future solve-island condensation can respect the two axes independently.

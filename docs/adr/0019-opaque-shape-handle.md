# ADR 0019: Opaque OCCT shape wrapper

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

Domain APIs use a move/copy-safe opaque `ShapeHandle` implemented by the OCCT adapter. Public headers expose no `TopoDS_Shape`; adapter access requires an explicit boundary conversion.

## Consequences

OCCT ABI and headers do not infect the domain. Geometry remains cheap to share while ownership follows RAII and no owning raw pointer is introduced.

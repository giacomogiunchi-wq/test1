# ADR 0007: PlaneGCS behind ISketchSolver

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

The sketch domain and constraint vocabulary are Duomec-owned. A pinned PlaneGCS extraction implements `ISketchSolver` and translates at the boundary.

## Consequences

FreeCAD coupling and ABI changes are localized; another deterministic solver can replace it without rewriting features or UI.

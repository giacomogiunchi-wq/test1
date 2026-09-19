# ADR 0028: Repeated-component occurrence representation

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

An occurrence stores a stable part/revision reference, transform, visibility, and occurrence metadata. Exact geometry and tessellation belong to the unique referenced revision, never each occurrence.

## Consequences

Large repeated structures avoid deep copies. Display-only load is possible before exact part geometry is resident; full assembly behavior remains out of M2.

# ADR 0022: Multibody ownership and result semantics

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

A later `PartDocument` owns a heterogeneous body collection. Features declare an operation mode and may produce zero, one, or many body results with provenance; target bodies are explicit and never inferred silently.

## Consequences

Feature identity and body identity remain distinct. NewBody/Add/Cut/Intersect share one vocabulary now, avoiding a single-feature/single-body assumption in M2B.

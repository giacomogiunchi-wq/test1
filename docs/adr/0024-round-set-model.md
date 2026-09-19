# ADR 0024: Round and fillet-set data model

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Represent a round as ordered sets containing persistent edge references, propagation, radius law, transition policy, and stop policy. Execution is all-or-diagnosed; partial output is never silently accepted.

## Consequences

Constant radii can ship first while variable laws and transition choices extend the same persisted schema. Kernel limitations are isolated and attributable per set/edge.

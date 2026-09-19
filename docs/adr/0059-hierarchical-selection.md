# ADR 0059: Hierarchical selection

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Selection first resolves an occurrence using assembly bounds/BVH, then loads or reuses detailed local selection data only for candidates. Local structures are keyed by definition/body revision and transformed at query time.

## Consequences

Global face/edge/vertex activation and per-occurrence detailed duplication are avoided.

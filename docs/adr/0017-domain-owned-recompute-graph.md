# ADR 0017: Domain-owned recompute graph

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

A Duomec DAG owns feature dependencies, dirty propagation, cycle detection, topological ordering, blocked states, and stale-result semantics. OCAF persists dependencies and provides transactions but does not define application scheduling.

## Consequences

Recompute policy is deterministic and unit-testable without OCAF. Adapter callbacks execute drivers but cannot create hidden dependencies.

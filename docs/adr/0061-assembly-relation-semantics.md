# ADR 0061: Preserve assembly relation semantics

* Status: Accepted
* Date: 2026-09-19

## Context

Future assembly solvers need equations, while repair, UI, motion export, and analysis need the user's semantic intent and persistent references.

## Decision

Duomec owns `AssemblyRelation`, endpoints, topology identities, compact geometry, frames, parameters, state, DOF result, and optional motion/analysis semantics. Solver adapters may derive equations but may not replace the persistent relation with anonymous equations.

## Consequences

No solver type crosses the domain boundary. A future adapter can be replaced without migrating authoritative assembly semantics.

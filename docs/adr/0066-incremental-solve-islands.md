# ADR 0066: Incremental assembly solve islands

* Status: Accepted
* Date: 2026-09-19

## Context

Solving unrelated occurrences violates the M5.1 interaction budget.

## Decision

`AssemblyRelationGraph` maintains occurrence-to-island and island-to-index maps. Adds create or merge only touched islands. Removal discards and partitions only the previously connected island; unrelated islands remain intact. Drag requests copy only the selected connected island into the solver.

## Consequences

Solver input size follows local mechanism size rather than assembly size. Island identities may change after a split and are runtime accelerators, not semantic relation identity.

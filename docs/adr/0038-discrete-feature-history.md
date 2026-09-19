# ADR 0038: Mesh and point-cloud feature history

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Discrete processing is an ordered non-destructive history. Each feature has a stable ID, typed operation, parameters, suppression state, input/output revisions, cache key, and state. Editing or suppression invalidates only that feature and downstream entries.

## Consequences

Source snapshots remain unchanged; processing can be cached and replayed. Linear histories ship in M4A while the record format can later carry DAG dependencies.

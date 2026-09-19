# ADR 0015: OCAF transactions behind application commands

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

Keep the Duomec command/application API, while the document store opens, commits, aborts, undoes, and redoes OCAF commands. One successful user mutation is one OCAF undo unit including its projection/recompute; a failure aborts it.

## Consequences

UI remains independent of OCAF and undo restores domain state by rehydrating the current OCAF projection. Commands cannot partially commit.

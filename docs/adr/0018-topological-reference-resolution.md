# ADR 0018: Topological reference resolution

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

Persist a Duomec reference ID, producer, expected topology kind, historical token, geometric signature, adjacency signature, and optional intent. Resolve in that order through TNaming then deterministic scored fallbacks; tied/insufficient candidates are explicit errors.

## Consequences

Simple edits can retain intent without raw explorer indices. The application does not promise perfect naming and never silently chooses an ambiguous candidate.

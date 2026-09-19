# ADR 0012: OCAF document schema

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

Use a fixed, versioned label layout made only from standard OCAF attributes in 1A. UUID attributes identify domain objects; label tags and entries are private adapter details. Reserve feature children for later real dependencies, references, functions, results, and diagnostics rather than persisting placeholders.

## Consequences

Schema migration is explicit and custom persistence drivers are avoided. The label projection can change internally without changing application identity.

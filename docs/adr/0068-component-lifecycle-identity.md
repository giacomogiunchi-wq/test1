# ADR 0068: Component lifecycle preserves occurrence identity

* Status: Accepted
* Date: 2026-09-20

## Context

Insertion, replacement, virtualization, extraction, hierarchy changes, and independence must not invalidate occurrence-scoped state or duplicate shared geometry.

## Decision

Lifecycle commands mutate compact definition references and hierarchy records. `OccurrenceId`, transform, visibility, suppression, appearance, and metadata remain occurrence-owned. Replacement accepts part/subassembly definitions interchangeably. Make Independent clones the logical definition ID using copy-on-write while retaining identical content-addressed asset hashes until an edit creates a new revision.

## Consequences

No command in Step 5.2.3 loads B-Rep, retessellates, or rewrites unrelated definitions. Definition-kind changes do not imply occurrence replacement.

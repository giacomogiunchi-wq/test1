# ADR 0042: Discrete region and provenance model

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Every derived region/entity references source asset hash, source revision, source element domain/range, producing feature, parameters, and validation state. Region labels are stable Duomec IDs; backend cluster indices are transient.

## Consequences

Refitting and visual comparison can trace results to immutable input. Backend reordering cannot silently change identity.

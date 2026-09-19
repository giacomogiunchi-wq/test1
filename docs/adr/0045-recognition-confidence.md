# ADR 0045: Recognition confidence and evidence

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Recognition confidence is a classified outcome derived from persisted evidence contributions and residual validation, never an unexplained scalar. Candidates list supporting/contradicting evidence, source references, thresholds, and warnings.

## Consequences

Users can audit uncertainty and test corpora can report false positives/negatives. ML may add evidence later but cannot replace deterministic validation.

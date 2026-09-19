# ADR 0041: Native versus Analysis Situs recognition

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Define future `IBRepRecognizer` around a Duomec adjacency/evidence model. Start with deterministic native recognition; evaluate only license-reviewed Analysis Situs open-core components through an optional adapter. Never import its application/data model or commercial extensions.

## Consequences

Candidates remain portable and explainable. License uncertainty cannot block core builds, and corpus results can compare recognizers.

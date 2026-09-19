# ADR 0021: Feature preview lifecycle

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

A `FeaturePreviewSession` owns draft values, preselection, cancellation, latest transient result, and terminal state. Preview runs through `IFeatureExecutor` at explicit quality; commit reruns final quality through `IFeatureTransaction` exactly once; cancel never reaches persistence.

## Consequences

Transient shapes remain outside the authoritative document. Slow executors must observe the supplied stop token; the UI can replace or cancel previews without undo pollution.

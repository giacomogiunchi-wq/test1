# ADR 0074: Relation diagnostics and repair are explicit

* Status: Accepted
* Date: 2026-09-20

## Context

A generic solve failure is insufficient, and automatic nearest-geometry attachment can silently corrupt design intent after topology changes.

## Decision

Diagnostics preserve distinct Solved, Underconstrained, Redundant, Conflicting, DanglingReference, Suppressed, SolverFailed, and NeedsReview states. When structural information permits, diagnostics return the smallest known conflicting relation pair. Repair first attempts persistent-reference recovery, then compatible descriptor/signature candidates. A unique fallback may be previewed; ambiguity becomes NeedsReview and requires an explicit user selection before mutation.

## Consequences

The dependency-free classifier cannot always produce a globally minimal conflict set, so it reports the smallest set it can prove. No arbitrary nearest entity is selected.

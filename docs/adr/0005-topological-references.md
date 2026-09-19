# ADR 0005: Composite topological references

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Persistent references combine OCAF/TNaming history, geometric signature, adjacency context, and confidence-ranked fallback re-identification. Raw face or edge indices are forbidden.

## Consequences

Recompute survives more edits, while ambiguous rebinding becomes an explicit diagnostic rather than hidden corruption.

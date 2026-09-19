# ADR 0025: SurfaceBody representation

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Surface bodies are first-class bodies with oriented surface/shell geometry, boundary references, provenance, and G0/G1/G2-ready continuity intent. They are not invalid solids.

## Consequences

Surface workflows can preserve intent through trim/sew/thicken and use dedicated validation. Exact geometry stays opaque behind the OCCT adapter.

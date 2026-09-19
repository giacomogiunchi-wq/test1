# ADR 0057: Independent edge-cache architecture

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Surface meshes and classified edge polylines are distinct versioned assets with independent LOD and budgets. Occurrence transforms never regenerate local edge geometry.

## Consequences

CAD edge cost can degrade during navigation and recover after settle without remeshing or changing engineering state.

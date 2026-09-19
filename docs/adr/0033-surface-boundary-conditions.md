# ADR 0033: Surface boundary-condition model

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

Each future surface boundary stores its persistent topology reference, requested continuity, optional support face, optional influence, orientation, and explicit ordering. Build settings and measured result quality are stored separately.

## Consequences

Fill/loft UI can promote individual boundaries from position to tangent/curvature without losing intent. Broken support references fail explicitly rather than degrading to G0.

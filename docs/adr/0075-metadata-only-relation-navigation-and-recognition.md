# ADR 0075: Relation navigation and joint recognition use metadata

* Status: Accepted
* Date: 2026-09-20

## Context

Relation browsing, selected-component views, and joint recognition must remain usable in large assemblies without forcing exact geometry residency.

## Decision

Relation views and browser groups operate on occurrence IDs, relation state/type, folders, and solve-island IDs. Recognition inspects semantic relation patterns and compact descriptors. It returns candidates only; `KinematicJointComposer` rewrites or composes semantics solely after explicit confirmation.

## Consequences

Listing, filtering, fading, solve-island isolation, and candidate recognition do not load B-Rep. Viewport glyph rendering remains a UI integration concern.

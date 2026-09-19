# ADR 0064: Motion and analysis semantics are lightweight hints

* Status: Accepted
* Date: 2026-09-19

## Context

Assembly intent may later seed Project Chrono joints or FEM setup, but CAD relations are not runtime physics objects and a CAD mate is not necessarily physical contact.

## Decision

Relations persist solver-neutral `MotionSemanticDescriptor` metadata and an optional `AnalysisRelationHint`. No Chrono or FEM object is created in the assembly runtime. Analysis hints require downstream interpretation and may record explicit user acceptance.

## Consequences

Motion and analysis adapters can consume stable frames and semantic intent later without adding heavyweight dependencies or silently asserting FEM contact.

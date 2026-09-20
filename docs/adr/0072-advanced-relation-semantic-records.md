# ADR 0072: Advanced relations remain semantic records

* Status: Accepted
* Date: 2026-09-20

## Context

Limits, paths, profiles, symmetry, and width constraints lose authoring intent if they are persisted only as anonymous scalar equations. Their interactive hot path must nevertheless remain independent of B-Rep and display data.

## Decision

Persist each advanced relation as a typed `AssemblyRelation` with compact endpoints, local frames, mode values, and parameters. Limits retain ranges and current values; paths retain persistent topology references and parametric position; Profile Center and Width retain their high-level mode. Solver adapters may compile these records transiently but may not replace them in storage.

## Consequences

Save/reload, repair, diagnostics, and future solver adapters retain user intent. Geometry compatibility is validated when a relation is authored or repaired; solving consumes descriptors and frames only.

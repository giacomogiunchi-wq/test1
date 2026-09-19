# ADR 0037: PointCloudBody representation

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Point clouds are first-class bodies with immutable point snapshots, stable IDs, statistics, source transform, asset reference, and revision. They are never represented as meshes or B-Reps implicitly. Explicit history features create every derived representation.

## Consequences

Scan precision, colors, normals, transforms, and provenance survive independently. Conversion and reconstruction remain auditable operations.

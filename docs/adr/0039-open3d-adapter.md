# ADR 0039: Open3D adapter boundary

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Open3D is the intended primary M4 processing backend behind `IMeshKernel` and `IPointCloudKernel`. Open3D objects never enter domain headers. The adapter is disabled until an exact recipe, ABI policy, and integration corpus are reviewed.

## Consequences

A lightweight native OBJ/XYZ adapter tests contracts now without pretending to provide Open3D algorithms. Backend replacement remains possible.

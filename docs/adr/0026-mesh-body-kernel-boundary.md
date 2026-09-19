# ADR 0026: MeshBody and IMeshKernel boundary

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Mesh bodies use Duomec vertex/index/region IDs and metadata, separate from B-Rep topology. `IMeshKernel` owns import, validation, normals, transforms, cleanup, and decimation contracts; VTK is the initial adapter and Open3D remains optional evaluation.

## Consequences

Neither VTK nor Open3D types enter domain APIs. Mesh persistence and selection can evolve independently from exact feature history.

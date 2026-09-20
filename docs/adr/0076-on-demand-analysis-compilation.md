# ADR 0076: Motion and FEM models compile on demand

* Status: Accepted
* Date: 2026-09-20

## Context

Assembly relations must be reusable by future Motion and FEM environments without adding their objects, geometry residency, or invalidation costs to normal assembly work.

## Decision

Duomec exposes domain-owned `IKinematicModelCompiler`, `IMultibodyRelationExporter`, and `IAssemblyToFemCompiler` boundaries. Compilers accept an explicit occurrence scope and immutable relation metadata. Exact geometry is available only to the FEM compiler through `IAnalysisGeometryResolver`; the Motion compiler has no geometry-resolver dependency. Compiled models live in separate scope-aware analysis caches.

## Consequences

No Project Chrono, Code_Aster, mesh, contact, or connector objects exist until explicit compilation. An edit outside a cache entry's analysis scope does not invalidate it. Backend adapters remain future work.

# ADR 0027: Storage abstraction and cache separation

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Persist authoring data, exact-geometry cache, tessellation cache, metadata, external references, and occurrence records as separate logical streams behind ports. Cache keys include stable identity, revision, representation parameters, and LOD.

## Consequences

OCAF may remain an authoring backend but is not the public file contract. Corrupt or stale caches can be discarded without losing editable intent.

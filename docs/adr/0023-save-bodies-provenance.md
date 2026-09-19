# ADR 0023: Save Bodies provenance and external files

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Exported bodies will use the document-storage port and carry source document/body/feature IDs, source revision hash, coordinate system, units, material, and name. Deterministic names are proposed before atomic writes; overwrite requires explicit policy.

## Consequences

A saved body is independently loadable and auditable. Associative updating can later use provenance without treating an absolute path as identity.

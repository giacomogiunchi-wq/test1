# ADR 0047: Definition, revision, and occurrence model

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

A definition identifies a part lineage, an immutable revision identifies committed content hashes, and an occurrence stores hierarchy, transform, visibility, suppression, appearance override, and metadata. Occurrences never own deep geometry copies.

## Consequences

Repeated components share geometry/display assets; instance operations remain compact and transform-only.

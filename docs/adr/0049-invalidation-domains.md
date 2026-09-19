# ADR 0049: Granular invalidation domains

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Track authoring, body geometry, topology signature, display geometry, material, appearance, and metadata hashes separately. Operations publish explicit invalidation domains.

## Consequences

Rename, transform, appearance, and body edits invalidate only their true dependents; blanket cache invalidation is testable as an error.

# ADR 0052: SQLite local-catalog policy

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

SQLite may index a local per-user cache and performance/LRU metadata. It is never the authoritative network-shared CAD container, and WAL is not placed on a network filesystem.

## Consequences

Transactional local lookup is available without assuming cross-host shared-memory semantics.

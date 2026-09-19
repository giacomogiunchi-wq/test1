# ADR 0051: Source storage versus local cache

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Authoritative local/NAS/synced files remain source truth. A disposable per-user local cache is keyed by verified content; network locks do not protect local cache correctness.

## Consequences

Unchanged remote content is fetched once while deleting the cache cannot destroy authored state.

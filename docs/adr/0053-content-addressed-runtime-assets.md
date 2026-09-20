# ADR 0053: Content-addressed runtime assets

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Derived surface, edge, selection, proxy, and mass assets use keys composed from exact input hashes plus versioned generation policy. Cache entries are immutable and verified before reuse.

## Consequences

Assets share across tabs, occurrences, sessions, and assemblies; policy changes cannot alias stale output.

## M5.1C implementation note

`CacheKey` combines asset kind, verified definition-level content hash, format
version, generator version, and settings identity. Occurrence transform and
visibility are intentionally excluded.

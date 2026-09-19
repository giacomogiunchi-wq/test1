# ADR 0048: Immutable revision and copy-on-write policy

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Committed revision assets are immutable. Editing creates a working authoring copy and commit creates a new revision; tasks and sessions retain old revisions safely.

## Consequences

Undo, comparison, shared caches, and stale-task rejection do not race against in-place geometry mutation.

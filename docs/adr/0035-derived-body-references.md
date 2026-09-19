# ADR 0035: Derived-body external references

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

Associative derived bodies reference source document/body IDs plus immutable source revision hash and transform. Resolver status is resolved, stale, missing, ambiguous, or cyclic. Updates are explicit transactions and retain the last valid snapshot as visibly stale when a source is unavailable.

## Consequences

Paths are locators rather than identity. Only affected links invalidate; cycles and missing sources are non-destructive explicit states.

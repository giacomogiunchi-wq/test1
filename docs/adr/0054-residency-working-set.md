# ADR 0054: Residency and working-set policy

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Represent metadata, bounds, proxy, full display, exact geometry, and authoring as explicit capabilities. HOT/WARM/COLD policy uses budgets and never evicts dirty authoring state.

## Consequences

Progressive open and predictable memory become controllable rather than side effects of object construction.

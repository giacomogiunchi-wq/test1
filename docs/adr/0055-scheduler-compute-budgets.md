# ADR 0055: Central scheduler and compute budgets

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

One Duomec scheduler owns priorities, cancellation, dependencies, revision tokens, bounded queues, and compute budgets. Completion commits only if its source revision still matches.

## Consequences

Interaction can preempt warming, stale jobs are harmless, and nested OCCT parallelism cannot oversubscribe unnoticed.

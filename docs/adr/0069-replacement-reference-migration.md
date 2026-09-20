# ADR 0069: Ordered and diagnosable replacement reference migration

* Status: Accepted
* Date: 2026-09-20

## Context

Silently dropping or geometrically guessing mates during replacement corrupts assembly intent.

## Decision

Reference mapping proceeds through published Mate References, absolute references, stable names, compatible descriptors, and topology signatures. Exact semantic mappings are Resolved; descriptor/signature fallbacks are NeedsReview; absent and incompatible matches remain explicit. The outcome is persisted on each affected relation and returned in a replacement report.

## Consequences

Failed mappings never disappear or arbitrarily attach. Manual repair remains possible because occurrence, relation, endpoint, and original semantic records survive.

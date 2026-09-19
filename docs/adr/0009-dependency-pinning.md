# ADR 0009: Pinned dependency baselines

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Release recipes pin exact versions and checksums. Upgrades require compatibility tests, license review, and an ADR amendment. Runtime solver adapters validate detected major versions.

## Consequences

Builds and numerical baselines stay reproducible instead of changing with ambient system packages.

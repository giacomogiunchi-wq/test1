# ADR 0006: Feature drivers and dependency graph

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Parametric features are independent drivers returning `FeatureResult`; a directed dependency graph schedules downstream recompute inside OCAF transactions.

## Consequences

Qt contains no feature logic. Changed/generated/deleted topology history and warnings travel with each computation.

# ADR 0004: SI internal units

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Store all physical values in SI in domain models. UI and import/export boundaries perform explicit conversion through the unit service.

## Consequences

Solver coupling becomes predictable. UI display units remain configurable without contaminating studies or geometry semantics.

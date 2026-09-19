# ADR 0008: One managed external-process runner

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Process adapters share a runner providing argv-safe launch, version probes, structured logs, timeouts, cancellation, manifests, and content-addressed deterministic work directories.

## Consequences

Code_Aster and OpenFOAM integrations gain consistent observability and lifecycle behavior on supported deployment targets.

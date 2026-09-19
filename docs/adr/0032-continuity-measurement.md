# ADR 0032: Continuity measurement

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

Store requested G0/G1/G2 intent separately from sampled measurements. A kernel-neutral evaluator samples a normalized shared boundary, reports maximum, mean, RMS, sample location, and availability for position, tangent-plane angle, and curvature. Adapter-provided immutable evaluators own parameter correspondence.

## Consequences

An API continuity setting is not treated as proof. Sampling is deterministic at fixed count and can become adaptive without changing result vocabulary; reports disclose unavailable curvature.

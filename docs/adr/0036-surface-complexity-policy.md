# ADR 0036: Surface complexity and file-weight policy

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

Record degree, poles, knots, spans, rational/periodic flags, bounding box, tolerances, and deterministic approximate payload bytes for generated curves/surfaces. Future build policies cap defaults, distinguish preview/final quality, preserve analytic forms, and warn before excessive approximation or tolerance growth.

## Consequences

Quality regressions become measurable. Byte estimates compare geometric complexity consistently but are not serialized file-size promises; exact cache and tessellation sizes remain separate metrics.

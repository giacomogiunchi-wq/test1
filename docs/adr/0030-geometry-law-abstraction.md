# ADR 0030: Geometry law abstraction

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

Scalar laws are immutable Duomec domain objects over normalized parameter `[0,1]`. Stations are finite, strictly ordered, and include both endpoints. Piecewise-linear interpolation is the predictable default; cubic interpolation is explicit. Radius/scale/twist wrappers enforce semantic value constraints. A versioned canonical codec persists laws; an adapter later converts them to OCCT `Law_Function`.

## Consequences

Laws can be tested and persisted without OCCT and cannot inherit kernel object identity. Extrapolation is clamped and documented. Changing interpolation never happens implicitly.

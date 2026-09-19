# ADR 0031: Round orchestration and fallback

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

M3B round execution will orchestrate ordered `IRoundStrategy` implementations and preserve the requested radius law throughout. Native OCCT is first; split/local strategies may produce diagnosed alternatives. Reduced-radius execution is diagnostic probing only and cannot be committed as the requested design. `BadShape` is never accepted as a final result.

## Consequences

Failures can identify a strategy, set, contour, vertex, and partial state. Fallback improves diagnosis without silently weakening intent.

# ADR 0003: Explicit Result and error values

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Operations that can fail return `core::Result<T>` carrying a categorized error, message, and context. Exceptions are contained at third-party boundaries.

## Consequences

Geometry and solver failures cannot be silently ignored; callers must branch on success and can log stable diagnostic categories.

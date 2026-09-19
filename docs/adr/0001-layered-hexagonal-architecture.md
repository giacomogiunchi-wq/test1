# ADR 0001: Layered hexagonal architecture

* Status: Accepted
* Date: 2026-09-19

## Context

The platform needs a durable decision for this concern before feature expansion.

## Decision

Duomec domain models and ports form the inward-facing core. UI, scripting, and engine adapters depend inward; no subsystem calls an engine except through its adapter.

## Consequences

This preserves replaceable solvers and prevents third-party types from becoming the application model, at the cost of explicit translation code.

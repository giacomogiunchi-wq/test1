# ADR 0056: Assembly render-backend abstraction

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Assembly runtime targets `IAssemblyRenderBackend`; AIS objects stay in adapters. Benchmark OCCT connected presentations before considering a hybrid instanced backend.

## Consequences

Exact authoring can remain OCCT while repeated background display may evolve without contaminating the product graph.

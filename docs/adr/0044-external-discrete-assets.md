# ADR 0044: External large-asset storage

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Raw mesh/scan payloads are content-addressed external assets by default. The document persists hash, relative locator, byte size, units, transform, format, and processing revision; working and display caches are independently disposable.

## Consequences

Large data does not inflate authoring metadata. Missing/corrupt assets are explicit, and relocation can use hashes rather than absolute paths.

# ADR 0029: Large-model benchmark methodology

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Use deterministic generated datasets, fixed seeds, warm-up disclosure, repeated samples, checksummed work, and machine/toolchain metadata. Record medians plus ranges for file size, cold/warm open, peak RAM, tessellation, edit/recompute, object count, and save.

## Consequences

Numbers are engineering baselines rather than marketing promises. CI may use generous regression envelopes while dedicated runners retain comparable historical results.

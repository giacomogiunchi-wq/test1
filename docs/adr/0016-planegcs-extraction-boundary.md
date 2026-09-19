# ADR 0016: PlaneGCS extraction and license boundary

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

In 1B, vendor a reviewed minimal source snapshot pinned to an exact FreeCAD commit. Preserve per-file LGPL notices and publish modifications. Only `PlaneGCSAdapter` may include solver headers; Duomec sketch variants serialize without it.

## Consequences

A solver upgrade or replacement is localized. Extraction and license inventory are release gates, not assumptions based on the repository-level license.

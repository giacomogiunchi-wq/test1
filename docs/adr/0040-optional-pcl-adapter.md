# ADR 0040: Optional PCL adapter

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

PCL is an optional point-cloud plugin for RANSAC, segmentation, clustering, and scan workflows where it materially outperforms the primary backend. Core domain and saved documents cannot require PCL-specific types or algorithm IDs.

## Consequences

Install size and overlapping dependencies do not burden default builds. Results must translate into the same Duomec metrics and provenance.

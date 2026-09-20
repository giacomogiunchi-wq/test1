# ADR 0077: FEM hints and joint mappings require review

* Status: Accepted
* Date: 2026-09-20

## Context

CAD placement intent is not sufficient evidence for a physical contact, bonded interface, bearing, or structural boundary condition.

## Decision

The FEM extractor emits review-required candidates only. It maps relation semantics to candidate categories and retains persistent endpoints, but never confirms a physical condition. Motion export prefers a confirmed high-level joint over redundant low-level relations on the same bodies and validates stable local reaction frames.

## Consequences

Future Motion/FEM environments own confirmation, material/contact choices, meshing, backend objects, and solve execution. Normal assembly state remains authoritative and backend-neutral.

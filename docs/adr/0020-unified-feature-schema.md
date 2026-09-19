# ADR 0020: Unified feature-schema architecture

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 2 requires a stable cross-feature decision before production features are added.

## Decision

Feature commands expose immutable Duomec `FeatureDefinition` data: ordered panel groups, typed parameters, collectors, supported result modes, preview capability, and manipulators. Qt renders that schema and never owns modeling semantics.

## Consequences

Create and edit reuse one definition and context vocabulary. Adding a feature does not require a bespoke semantic dialog, though specialized UI renderers may enhance a common schema.

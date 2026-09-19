# ADR 0034: Direct-edit feature representation

* Status: Accepted
* Date: 2026-09-19

## Context

Advanced geometry needs this decision before kernel-specific feature work begins.

## Decision

Every accepted direct edit is an ordinary parametric `DirectEditFeature` containing operation kind, persistent face references, typed parameters, preflight report, and topology evolution. No command mutates the authoritative body outside history.

## Consequences

Undo/recompute/reference updates use the normal feature lifecycle. Unsupported freeform edits fail before commit and do not become hidden destructive changes.

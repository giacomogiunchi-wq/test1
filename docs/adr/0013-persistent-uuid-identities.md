# ADR 0013: Persistent UUID identities

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

Represent every persistent identity as a distinct strongly typed Duomec ID holding a canonical RFC 4122 UUID string. Generate UUIDv4 values in the domain and validate loaded values. Never expose `TDF_Label` entries as identity.

## Consequences

Accidental cross-kind ID use is rejected at compile time and save/load preserves identity independently from label placement.

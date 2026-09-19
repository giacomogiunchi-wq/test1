# ADR 0014: Binary OCAF persistence

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 1 requires this decision before its implementation can safely expand.

## Decision

Use registered `BinOcaf` documents with `.duomec`, an independent integer schema version, and atomic save by temporary write, validation reopen, then rename. Use only standard attributes until a demonstrated need justifies a custom driver.

## Consequences

Files contain the feature model rather than only B-Reps. Interrupted writes do not replace the last valid target; migrations become necessary when the schema version changes.

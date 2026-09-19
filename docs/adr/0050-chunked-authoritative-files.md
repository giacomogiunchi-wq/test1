# ADR 0050: Chunked authoritative file format

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Future `.duomecpart` and `.duomecasm` containers use a versioned header/TOC and independently checksummed chunks. Unknown optional chunks are skippable; atomic replacement validates the new directory and checksums.

## Consequences

Random access and localized corruption are possible without treating derived caches as authoritative.

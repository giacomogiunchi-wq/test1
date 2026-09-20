# ADR 0070: Form Subassembly is a hierarchy and ownership rewrite

* Status: Accepted
* Date: 2026-09-20

## Context

Forming a subassembly must preserve world positions, relations, and derived geometry caches.

## Decision

The new subassembly frame equals the current parent frame and its occurrence transform is identity. Selected same-level occurrences retain their local transforms and are reparented. Relations whose endpoints are all selected move to the new definition owner; cross-boundary relations remain parent-owned and retain endpoint occurrence IDs.

## Consequences

Visible geometry does not move, and no geometry or relation is rebuilt. External creation uses an atomic virtual-to-external export and rolls the snapshot back if export fails.

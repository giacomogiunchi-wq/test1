# ADR 0058: Display-only LOD policy

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Fine/normal/coarse/proxy/bounds representations are display capabilities selected with hysteresis, budgets, interaction state, and selected/editing quality floors.

## Consequences

LOD cannot silently participate in exact calculations, and camera motion does not invalidate B-Rep.

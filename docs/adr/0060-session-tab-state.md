# ADR 0060: Session and tab architecture

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Per-tab camera, selection, tree, style, representation, and edit context live in lightweight session state. Exact/display/GPU caches are shared services outside tabs.

## Consequences

Tab switching changes references/state rather than rereading, retessellating, or recreating unchanged buffers.

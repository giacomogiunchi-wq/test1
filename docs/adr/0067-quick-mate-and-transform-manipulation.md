# ADR 0067: Quick Mate and transform-only manipulation

* Status: Accepted
* Date: 2026-09-19

## Context

Common mate creation and component dragging must not invoke authoring/B-Rep or global solves.

## Decision

Quick Mate candidate selection operates on `GeometryDescriptor` kinds and produces semantic `AssemblyRelation` records. Its overlay is renderer/UI-neutral state; a future Qt view binds near-cursor presentation and Enter/Tab/Esc events to the controller. Manipulation changes occurrence transforms only, uses DOF projection, groups selections transiently, and executes island-local interactive/final solver calls with revision checks.

## Consequences

No Qt, AIS, tessellation, or topology object enters the hot path. This step provides controller behavior, not the Step 5.2.3 production insertion or component-lifecycle UI.

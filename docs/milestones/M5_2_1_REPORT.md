# Milestone 5.2 Step 5.2.1 report

## Implemented

- Persistent typed IDs for definitions, occurrences, relations, endpoints, kinematic frames, and future solve islands.
- Lightweight absolute references for Origin, Front/Top/Right, X/Y/Z, and primary coordinate frame.
- Orthogonal Fixed/Floating mobility and Rigid/Flexible subassembly mode.
- Solver-neutral six-axis DOF state and distinct redundant/conflicting classifications.
- Compact point/line/axis/plane/circle/cylinder/cone/sphere/frame/curve/path/slot/unknown descriptors.
- Component-local right-handed frame convention: Z principal axis/normal, X orientation, Y completes the frame; orthonormal validation is provided.
- Origin-aligned identity-transform, Fixed default insertion without synthetic relations; interactive placement metadata defaults Floating.
- Lightweight future motion semantics and explicitly non-authoritative FEM hints.
- Versioned snapshot codec covering all Step 5.2.1 values.

## Architecture and performance

The new target depends on the document identity library only. It contains no OCCT, OCAF, Qt, solver, Project Chrono, or FEM types. Geometry descriptors cache solve-relevant local data while persistent topology IDs remain authoritative. Movement remains a transform update; this step adds no recompute or presentation path.

## Deferred by instruction

No mates, equations, solver integration, constrained dragging, component replacement, virtual components, subassembly creation, independence workflow, motion runtime, FEM runtime, or UI is implemented.

## Unresolved risks

- The M5.1B–J assembly runtime prerequisites remain absent.
- Snapshot persistence is currently a standalone schema; integration with the future chunked `.duomecasm` container and its undoable aggregate awaits M5.1C.
- Topology repair/re-resolution is not implemented, so descriptors and persistent references cannot yet be refreshed after part revision changes.
- Flexible subassembly solve-graph projection and solve-island ownership are intentionally only represented by metadata/IDs.

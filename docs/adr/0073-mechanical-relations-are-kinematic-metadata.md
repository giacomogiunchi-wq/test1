# ADR 0073: Mechanical relations are kinematic metadata

* Status: Accepted
* Date: 2026-09-20

## Context

Mechanical relationships must support assembly positioning now and future motion mapping later without introducing a multibody dynamics engine into the normal CAD runtime.

## Decision

Hinge, Gear, Rack and Pinion, Screw, Slot, Universal Joint, Cam, and Belt/Chain are domain-owned semantic records. They persist axes/frames, ratios, phases, directions, offsets, modes, and other compact metadata. Hinge maps to a Revolute motion semantic; other records retain enough data for later adapters. Cam stores follower/contact intent but performs no physical contact dynamics.

## Consequences

No Project Chrono, FEM, OCCT, tessellation, or UI types enter the relation model. The native backend can classify structural DOFs while future numerical backends compile the same records into backend equations.

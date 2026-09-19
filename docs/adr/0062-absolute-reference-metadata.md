# ADR 0062: Lightweight absolute reference metadata

* Status: Accepted
* Date: 2026-09-19

## Context

Origin and canonical planes/axes must be selectable before exact geometry or authoring history is resident.

## Decision

Part and assembly definition metadata each carry persistent identities for Origin, Front, Top, Right, X, Y, Z, and the primary coordinate frame. These identities live in lightweight metadata and do not imply B-Rep loading.

## Consequences

Origin-aligned insertion and later absolute-reference relations operate on metadata. Resolution to authoring or exact geometry remains an explicit later operation.

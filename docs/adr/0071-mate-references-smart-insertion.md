# ADR 0071: Mate References are lightweight insertion metadata

* Status: Accepted
* Date: 2026-09-20

## Context

Smart insertion must discover connection intent without loading feature history or exact geometry.

## Decision

Part definitions persist Primary/Secondary/Tertiary Mate References containing a persistent reference, preferred relation, alignment, local frame, descriptor, priority, and name. Preview matches descriptor/relation compatibility and returns transform/relation proposals; commit explicitly installs them.

## Consequences

Preview remains disposable and descriptor-only. A failed match creates no relation. Feature history and B-Rep residency are unaffected.

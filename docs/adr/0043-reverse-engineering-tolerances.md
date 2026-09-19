# ADR 0043: Reverse-engineering tolerance policy

* Status: Accepted
* Date: 2026-09-19

## Context

Milestone 4 needs this decision before discrete processing or recognition expands.

## Decision

Use a typed policy with separate SI values for source noise, merging, primitive/curve/surface fitting, sewing, and reconstruction validation. Named presets expose every number and are copied into feature parameters; there is no hidden global epsilon.

## Consequences

Reports state the tolerance that produced them. Changing a policy invalidates affected downstream work deterministically.

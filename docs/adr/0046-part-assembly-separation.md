# ADR 0046: Part authoring and assembly runtime separation

* Status: Accepted
* Date: 2026-09-19

## Context

M5.1 requires this boundary before assembly runtime optimization begins.

## Decision

Part authoring, assembly runtime, and derived display assets are separate domains. Assembly records reference committed part revisions and never recursively instantiate all authoring histories.

## Consequences

Large assemblies can become useful from manifests/proxies while exact and authoring data remain demand-loaded.

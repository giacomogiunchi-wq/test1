# Milestone 5.2 Step 5.2.3 report

## Lifecycle architecture

- `AssemblyLifecycleSnapshot` separates definitions, occurrences, relation semantics, and relation ownership.
- Occurrences retain identity, transform, visibility, suppression, appearance, metadata, mobility, and per-occurrence rigid/flexible mode.
- Definitions may be external or embedded virtual part/subassembly definitions with revision, authoring, materials, metadata, context references, lightweight connection catalogs, and content-addressed asset keys.
- Schema 1 lifecycle persistence embeds the existing assembly-relation schema and supports undo/redo snapshots.

## Operations

- Default insertion is identity/Fixed with no mates; explicit-pose insertion is Floating.
- Replacement supports selected, multi-selected, and all-instance replacement across part/subassembly kinds. Reference migration is ordered and every affected relation becomes Resolved, NeedsReview, DanglingReference, or Incompatible.
- Virtual components can be authored immediately and exported atomically without changing occurrences.
- Form Subassembly reparents same-level occurrences without changing transforms; internal relations change owner and cross-boundary relations remain parent-owned.
- Make Independent creates a new logical definition ID while retaining immutable asset hashes.
- Fix/Float and Rigid/Flexible modify independent occurrence properties without fake relations or hierarchy flattening.
- Mate References and smart-insertion preview/commit use lightweight descriptors and frames only.

## Cache and geometry behavior

No lifecycle API references OCCT, B-Rep, tessellation, AIS, or Qt. Replace and reparent operations touch selected occurrence metadata and affected relations only. Make Independent initially retains exact-geometry, display-mesh, edge, and selection-cache hashes byte-for-byte; later authoring revision changes are where copy-on-write assets diverge.

## Benchmark

Dependency-free runner, 10,000 repeated occurrences sharing one definition/asset set and 100 independent-copy trials:

| Metric | Result |
|---|---:|
| 10,000 default insertions | 831,736 µs |
| Unique asset sets after insertion | 1 |
| Make Independent median | 80,449 ns |
| Make Independent p95 | 116,439 ns |
| Make Independent p99 | 224,747 ns |

UUID generation dominates bulk insertion and needs improvement before a production performance gate. Results are baselines, not guarantees.

## Unresolved risks

- M5.1B–J runtime/container/cache systems remain absent, so asset sharing is represented by hashes rather than exercised against a real GPU/exact-geometry cache.
- External export writes the lifecycle metadata format; integration into the future chunked `.duomecpart`/`.duomecasm` container remains pending.
- Cross-boundary occurrence paths are represented by stable parent/occurrence IDs; a dedicated path index awaits the missing M5.1 hierarchy runtime.
- Descriptor/signature replacement fallbacks intentionally require review and do not perform topology repair.
- Smart insertion provides domain preview/controller data, not Step 5.2.4 or later viewport UI.

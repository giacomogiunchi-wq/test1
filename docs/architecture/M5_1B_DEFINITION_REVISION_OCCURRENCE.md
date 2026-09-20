# M5.1B definition/revision/occurrence runtime

## Reconciliation with the existing M5.2 domain

M5.2 already established the persistent `PartDefinitionId`,
`AssemblyDefinitionId`, and `OccurrenceId` types, `Transform`,
`PlacementMobility`, `SubassemblySolveMode`, relation endpoints, and the
component-lifecycle command/persistence contract. M5.1B reuses those types; it
does not introduce aliases or a competing identity family. It adds only the
missing persistent `PartRevisionId`, `BodyRevisionId`, and
`AssemblyRevisionId` UUID types.

The M5.2 `AssemblyLifecycleSnapshot`, `ComponentDefinition`, and
`ComponentOccurrence` remain the versioned lifecycle command/persistence DTOs
so schema-1/2 documents and all command behavior remain valid. They are not a
second runtime graph. `AssemblyRuntimeGraph` is the authoritative resolved,
committed runtime view. Lifecycle loading/commands can populate that view once
a caller has resolved legacy revision hashes to registry entries. This
controlled boundary avoids changing a released serialization schema merely to
embed runtime pointers. `OccurrenceState` supplied the already-tested mobility,
solve-mode, transform, and DOF semantics; the runtime occurrence reuses its
public enum and value types rather than redefining them.

Existing M5.2 relation endpoints continue to carry the same stable
`OccurrenceId`. The runtime graph owns an `AssemblyRelationsSnapshot`, so
Motion/FEM compilation and relation solving consume unchanged M5.2 records.
Replace Component corresponds to `replaceReference`; Make Independent creates
a new registry definition/revision and may retain the old content hashes;
Fix/Float and Rigid/Flexible remain occurrence state. The existing lifecycle
commands remain the compatibility command surface until a later persistence
migration is deliberately versioned.

## Ownership

```text
DefinitionRegistry
  +-- shared_ptr<const PartDefinition>
  +-- shared_ptr<const PartRevision> ----> BodyRevision identities/hashes
  +-- shared_ptr<const AssemblyDefinition>
  `-- shared_ptr<const AssemblyRevision>

AssemblyRuntimeGraph
  +-- shared_ptr<const DefinitionRegistry> (no reverse ownership)
  +-- AssemblyOccurrence[] -- compact committed reference only
  +-- parent/child indexes
  `-- existing M5.2 AssemblyRelationsSnapshot
```

Definitions are lightweight lineage/metadata records. Revisions contain only
immutable identities, role-specific hashes, lightweight metadata, and body
revision identities—never B-Rep, authoring objects, tessellation, Qt/AIS
objects, or GPU resources. An occurrence owns only hierarchy and instance
state. Fifty thousand occurrences can therefore hold fifty thousand compact
references/transforms while all resolve the same single shared immutable
revision object.

## Revision and hash contracts

Registry commit copies a revision into `shared_ptr<const PartRevision>` (or
assembly revision) and rejects duplicate IDs and mismatched
definition/revision pairs. Readers receive only const shared pointers.
Authoring begins from a committed snapshot and commit creates a fresh revision
ID; no API mutates a committed record.

Hash roles are strong, non-interchangeable C++ types:

* `AuthoringHash` changes when committed feature/authoring content changes.
* `GeometryHash` changes when exact shape content changes, but not for rename,
  occurrence transform, visibility, or appearance changes.
* `DisplayRelevantHash` changes when definition-level display-producing content
  changes; occurrence-only appearance overrides are not part of it.
* each `BodyRevision` independently identifies a body and its geometry hash.

M5.1B treats values as deterministic opaque content identities. Computing
canonical hashes from exact geometry is deliberately outside this layer.

Copy-on-write creates a new definition/revision lineage. Its geometry and
display hashes may initially equal the source, permitting future caches to
share content until a real edit commits divergent hashes.

## Hierarchy and mutation

The graph uses ordered indexes for stable ID lookup and parent-to-child access.
It supports validated add/remove, parent/child queries, root-to-leaf path
resolution, reference replacement, and cycle-safe reparenting. Reparenting
preserves occurrence identity and can preserve world transform using affine
matrix composition/inversion. Part and subassembly references are a type-safe
variant; subassemblies are not represented as fake parts.

Graph mutation is controlled through APIs. There is no mutable global state.
The graph itself is externally synchronized; future background tasks consume
immutable revision pointers rather than the mutable occurrence container.

## Invalidation

Mutation returns explicit `InvalidationEvent` domains. Transform updates emit
Transform, AssemblyBounds, and AssemblyRelation only. Visibility emits
Visibility and Selection. Appearance overrides emit Metadata and never exact
geometry or tessellation. Replacing a committed revision reference emits exact
geometry, tessellation, selection, mass-properties, relation, and bounds
domains scoped to the occurrence. The event is a contract for later caches;
M5.1B does not implement a cache.

## Deferred to M5.1C and later

M5.1B adds no chunked files, SQLite catalog, compression, disk/content cache,
residency or working-set manager, progressive loading, task scheduler, GPU or
edge cache, renderer/LOD/instancing, BVH/selection runtime, tab/session system,
or nonlinear solver. Persistent registry storage and migration of legacy
lifecycle revision-hash records into chunk manifests belong to M5.1C.

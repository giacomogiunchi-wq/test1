# M5.1C chunked authoritative storage and local cache

## Scope and ownership

M5.1C persists the existing M5.1B `DefinitionRegistry` and
`AssemblyRuntimeGraph`; it does not introduce alternate definitions, revisions,
occurrences, IDs, or content hashes. `.duomecpart` and `.duomecasm` are
Duomec-owned authoritative containers. The OCAF document store remains the CAD
authoring adapter: an OCAF binary can be carried as an opaque authoring chunk,
but no OCCT/OCAF type appears in the storage API and opening a container never
instantiates OCAF.

Authoritative data comprises engineering definitions, immutable revisions and
body identities, authoring payloads, exact geometry payloads, occurrence
hierarchy/state, relations, virtual definitions, and required design metadata.
The per-user local cache contains only rebuildable mesh, edge, selection, BVH,
LOD, proxy, or analysis assets. The source file never references the cache for
correctness. Removing the cache cannot remove engineering data.

## Binary container

All integers are explicitly encoded little-endian; compiler structs are never
written. The 72-byte fixed header is:

| Offset | Width | Field |
|---:|---:|---|
| 0 | 8 | `DUOMECC1` magic |
| 8 | 2 + 2 | format major/minor |
| 12 | 1 | part (1) or assembly (2) |
| 13 | 1 | little-endian marker (1) |
| 14 | 2 | reserved |
| 16 | 4 | future flags |
| 20 | 8 | TOC offset |
| 28 | 4 | TOC entry count |
| 32 | 36 | document UUID text |
| 68 | 4 | reserved header-integrity field |

The fixed 144-byte TOC record stores a 36-byte stable chunk key, 32-bit chunk
type and schema, 8-bit per-chunk codec and criticality, 64-bit payload offset,
stored size and uncompressed size, CRC32, and a length-delimited 64-byte
semantic content-hash field. CRC32 is storage-integrity metadata and is not an
M5.1B `ContentHash`.

Known chunks cover document metadata, definition/revision manifests,
occurrences, relations, opaque authoring data, opaque exact geometry, and
virtual-definition data. Unknown optional types are retained in the directory
and skipped. Unknown required types, unsupported major versions/codecs,
duplicate IDs, overlapping/out-of-range payloads, excessive counts/sizes,
truncation, and checksum mismatches fail explicitly. Central `StorageLimits`
bounds chunk counts, manifest/occurrence/relation counts, strings, and stored or
decompressed bytes before allocation.

`DuomecContainerReader::open` reads only the header and TOC. `findChunk` is
payload-free and `readChunk` seeks directly to one range using an independent
file stream. `bytesRead` demonstrates metadata-only behavior. Exact geometry or
authoring chunks are never read as an open side effect.

## Atomic save and codecs

Writers stream a same-directory uniquely named temporary, finalize the TOC and
header, flush (and `fsync` the temporary on POSIX), reopen it, validate the
directory and every known chunk checksum, close it, then atomically rename it.
A staged failure removes the temporary and leaves the previous target intact.

`None` is always available per chunk. `Zstd` is isolated behind
`DUOMEC_ENABLE_ZSTD`; disabled builds return `dependency_missing` rather than
pretending to compress. Metadata can remain uncompressed while callers choose
compression for suitable opaque payloads.

## Registry, assembly, and virtual definitions

The registry manifest round-trips part/assembly definitions, default revisions,
absolute references, metadata, immutable part/assembly revisions, strong hash
roles, body revisions, and definition-to-revision ownership. Definitions carry
an external/embedded-virtual storage classification with the same manifest
shape. Repeated occurrences serialize only their committed typed reference;
after reload they resolve through one registry-owned `shared_ptr<const
PartRevision>`.

The occurrence manifest preserves `OccurrenceId`, typed definition/revision
reference, parent, transform, visibility, suppression, appearance and metadata,
mobility, and subassembly solve mode. Relations use the existing versioned M5.2
serializer in a separate chunk. Parent ordering is reconstructed without using
runtime indices or pointer values.

## Disposable content-addressed cache

`CacheKey` consists of asset kind, definition-level content hash, asset format
version, generator version, and relevant settings identity. Occurrence
transform and visibility are absent. A geometry edit naturally selects a new
path; the old immutable entry remains reusable by older revisions.

`LocalAssetCache` shards deterministic paths by the first four encoded hash
characters, atomically inserts immutable entry files, validates embedded key
metadata and CRC32 on every get, and treats absent, corrupt, wrong-version, or
wrong-generator data as a miss. It provides get/put/validate/remove/clear and
statistics. Each instance synchronizes its mutations and has no global
singleton. Independent readers open immutable files independently.

`ICacheCatalog` contains only disposable lookup/performance metadata. The core
uses a thread-safe deterministic in-memory implementation. When
`DUOMEC_ENABLE_SQLITE_CACHE=ON`, `SqliteCacheCatalog` provides a local SQLite WAL
adapter behind the same interface; SQLite is never linked into or used by the
authoritative container.

## Failure and threading contracts

Readers are immutable after open except for per-handle bytes-read accounting;
callers use separate handles for concurrent jobs. Registry revisions remain
immutable. Cache catalog and insertion APIs synchronize their mutable state.
There is no hidden global storage or cache state.

A source checksum failure is an authoritative-document error naming the chunk.
A cache checksum failure is only a cache miss and removes the disposable entry.
This distinction is deliberate.

## Boundary with M5.1D

M5.1C provides random-access primitives only. It adds no residency states,
working-set or RAM/VRAM policy, scheduler, cancellation, prefetch, progressive
open policy, renderer/GPU cache, LOD selection, BVH selection, or session/tab
manager. M5.1D will decide which chunks/assets to request and when.

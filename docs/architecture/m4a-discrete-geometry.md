# M4A discrete geometry and scan infrastructure

## Representation

`MeshBody` and `PointCloudBody` are distinct domain types. Each owns metadata for an immutable source snapshot, an external asset reference, and a non-destructive processing history. Raw payload buffers are optional and shared as `shared_ptr<const ...>`; metadata-only reload intentionally leaves them absent. A mesh buffer contains vertices and triangle connectivity. A point buffer independently carries points, optional normals, and optional colors. Neither representation is a B-Rep.

A revision contains a monotonic sequence, content hash, and optional parent hash. The bootstrap native importers produce `fnv1a64:` hashes over exact asset bytes. FNV-1a detects accidental changes but is not collision resistant; production content-addressed storage must replace it with a reviewed cryptographic digest before assets are trusted across security boundaries.

## History and invalidation

Mesh and point-cloud pipelines store stable feature IDs, operation kind, typed parameters, enable/suppression state, input/output revisions, cache keys, and diagnostics. Editing or suppressing feature `N` clears outputs/cache keys and marks only `N..end` dirty or suppressed. Accepted results require a clean enabled upstream feature. Source buffers are never mutated.

The canonical `DUOMEC_DISCRETE 1` codec persists bodies, asset locators/hashes, statistics, transforms, and full histories using locale-independent `max_digits10` numbers. It deliberately excludes raw buffers. A decoded document must resolve and verify its external assets before processing.

## Adapter boundaries

`IMeshKernel` and `IPointCloudKernel` accept paths, explicit source-to-SI scale, `stop_token`, and progress callbacks, returning Duomec snapshots. M4A ships narrow standard-library OBJ and XYZ adapters to exercise the contract. OBJ supports vertices, positive/negative vertex references, slash-qualified references, and deterministic fan triangulation. It does not claim STL/PLY/Open3D production coverage.

Open3D is intended as the production mesh/point-cloud backend after an exact recipe and integration corpus are approved. VTK remains the display pipeline. PCL and Analysis Situs remain optional and disabled.

## Statistics

The native OBJ adapter reports exact stored counts, connectivity components, boundary/non-manifold edges, exact duplicate vertices, exactly degenerate triangles, orientation consistency, bounding box, area, conditional watertight volume, and payload bytes. Expensive self-intersection, near-duplicate, noise, spacing, and outlier analyses are intentionally deferred to backend processing features rather than guessed during import.

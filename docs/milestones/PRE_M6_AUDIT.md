# Pre-Milestone 6 repository audit

Date: 2026-09-20

## Repository and integration state

The audit began on branch `work` at `60470bc6f7994d432fc418a6e50cd87332eb5422` with a clean index and working tree. There is one local branch, no tags, no upstream tracking branch, and no configured Git remote. No merge, rebase, cherry-pick, unresolved index entry, or conflict marker was present. GitHub CLI is installed but unauthenticated, so pull requests, checks, branch protection, remote divergence, and stranded remote work could not be inspected. Empty `.gitkeep` files are intentional package-boundary markers; no generated build artifact is tracked.

## Code and architecture review

The implemented layers preserve Duomec-owned public types. OCAF/OCCT remain adapters, and assembly/domain headers contain no Qt, OCCT, Project Chrono, Code_Aster, B-Rep, AIS, or tessellation types. Component definitions own immutable asset hashes and authoring metadata; occurrences own identity, parent, placement, visibility, and overrides. Relation solving and browsing consume occurrence IDs, compact descriptors, and local frames. Motion/FEM compilation remains explicit and analysis caches remain separate.

The review also confirmed major *absent* areas: M5.1B–J storage, residency, renderer, GPU cache, selection/BVH, scheduler, tab/session, and background-task implementations do not exist in code. Consequently their concurrency, eviction, stale-task, GPU reuse, and viewport guarantees cannot be production-validated. The native assembly solver remains a structural DOF classifier rather than a nonlinear solver. These are milestone-readiness gaps, not hidden implementations.

## Correctness and stabilization fixes

- Fixed a logging deadlock: sinks are now copied under the mutex and invoked after releasing it, allowing a sink to replace itself safely.
- Fixed assembly-relation schema-1 compatibility: schema-2-only solve-island, Motion, and analysis-hint fields are now conditionally encoded/decoded.
- Advanced lifecycle persistence to schema 2 and now preserves complete `GeometryDescriptor` state, including secondary direction/radius, angle, and samples. Schema 1 remains readable.
- Hardened lifecycle parsing with named-section validation, collection/sample limits, and trailing-data rejection.
- Made Replace Component and Make Independent prevalidate source definitions/selections so invalid requests cannot leave partially mutated state.
- Made analysis-cache lookup revision-aware so a caller cannot reuse an entry for an incompatible source revision.
- Reworked relation-graph insertion to update a single touched island in place instead of erasing and rebuilding all indexes.
- Replaced per-ID `std::random_device` construction with a thread-local, once-seeded UUID-v4 generator and added concurrent collision coverage.
- Removed confirmed avoidable copies/allocations reported by clang-tidy in relation, profiler, feature, analysis, and discrete-processing paths.

## Test additions

Critical new coverage verifies reentrant logging, concurrent generation of 16,000 unique IDs, schema-1 compatibility, schema-2 complete descriptor round trips, malformed/trailing lifecycle rejection, mutation atomicity on invalid lifecycle requests, and revision-aware analysis-cache lookup.

## Performance results

Dependency-light Debug measurements, same benchmark shape:

| Operation | Before | After | Change |
|---|---:|---:|---:|
| One 300-occurrence island, median | 172,411 µs | 4,683 µs | 36.8× faster |
| 5,000-island graph construction | 1,778,716 µs | 358,966 µs | 5.0× faster |
| 10,000 repeated insertions | 883,239 µs | 80,249 µs | 11.0× faster |
| Create Subassembly, median | 611 µs | 68 µs | 9.0× faster |
| Make Independent, median | 50 µs | 25 µs | 2.0× faster |
| Origin/plane graph update, median | 39 µs | 13 µs | 3.0× faster |

The many-disconnected-island and relation-diagnostics paths remain allocation/container heavy. Benchmarks are engineering baselines, not product guarantees.

## Memory, cache, and threading findings

No owning raw pointer, hidden geometry singleton, or assembly-global mutable cache was found in the dependency-light domain code. Immutable asset sharing/copy-on-write is represented by content hashes. Analysis-cache invalidation is scoped by occurrence and lookup is now revision-checked. The repository has no implemented production residency manager, GPU cache, async scheduler, or background geometry worker, so cold eviction, dirty-authoring pinning, cancellation, OCCT worker safety, and stale asynchronous commits cannot yet be tested. Existing revision rejection in constrained drag remains covered.

## Persistence findings

Document, discrete history, assembly relations, lifecycle data, IDs, undo/redo, and analysis semantics have round-trip coverage. Lifecycle schema 2 fixes descriptor loss and accepts schema 1. Assembly relation schema 1 now represents its actual older layout. Runtime-derived analysis objects remain unserialized. The text lifecycle format rejects malformed section names, unreasonable counts, and trailing bytes. Unknown optional chunks are not supported because the promised chunked M5.1 container has not been implemented.

## Dependencies and licenses

Dependency baselines and license families remain documented. The default audited build uses only the standard library. Qt/OCCT/OCAF, pybind11, Chrono, Code_Aster, Gmsh, VTK, and other optional integrations were not available/enabled. No third-party solver or geometry type leaks into the audited domain APIs. Exact upstream commits/licenses for several disabled future adapters remain integration risks already recorded in `DEPENDENCIES.md` and `THIRD_PARTY_LICENSES.md`.

## Deferred technical debt classification

- **Must fix before Milestone 6:** implement/validate the M5.1B–J runtime prerequisites promised as complete; select and validate a production nonlinear assembly solver if Milestone 6 depends on solved mechanisms; establish authenticated remote/CI integration state.
- **Acceptable deferred:** viewport widgets/glyphs, actual Chrono/FEM adapters, optional PCL/Open3D/Analysis Situs integrations, and exact topology adapter work explicitly assigned to future milestones.
- **Obsolete/dead:** none proven. Empty package markers remain intentional extension boundaries.

## Readiness decision

**NO-GO FOR MILESTONE 6.** The audited dependency-light foundation is stable, but the repository itself documents and demonstrates that M5.1B–J runtime systems and a production nonlinear assembly solver are absent. GitHub/remote integration health is also unverifiable because no remote is configured and `gh` is unauthenticated. These gaps must be resolved or explicitly removed from the Milestone 6 entry criteria before development proceeds.

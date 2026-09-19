# Milestone 2 preflight and prerequisite mismatch

The dependency-free repository configured, built, and passed both available tests on 2026-09-19. The requested "complete Milestone 0/1 test suite" does not exist in this repository: only the Milestone 0 core tests and Milestone 1A document-domain tests are present. The OCAF integration target remains optional and cannot configure in the supplied runner because OCCT 8 is not installed.

Milestone 2's premise diverges materially from the repository:

1. No sketch domain, PlaneGCS adapter, sketch serialization, solver status, or degree-of-freedom implementation exists.
2. No dependency graph or recompute engine exists.
3. No concrete topological-reference resolver, TNaming integration, geometric signature, or ambiguity handling exists.
4. No Pad, Pocket, Revolve, Fillet, Chamfer, or Boolean feature driver exists.
5. OCAF 1A source exists, but its integration test has not been compiled in this environment. The earlier PR text claiming that test passed was inaccurate.
6. The Qt/OCCT viewport source likewise has not been built here because Qt 6 and OCCT 8 development packages are absent.
7. Feature persistence currently supports only one generic feature type and scalar parameters; it cannot yet persist M2 schemas, selections, extents, references, results, or multiple bodies.
8. The existing `GeometryShape` is only an ID placeholder; the opaque OCCT shape wrapper accepted by ADR 0019 has not been implemented.

M2A is therefore implemented strictly as a dependency-free interaction/execution framework with adapter/application transaction ports and test doubles. It does not claim integration with nonexistent M1B–M1G services. M2B must not begin until those prerequisites and the OCCT-enabled tests are restored or implemented.

The linked official reference sites could not be retrieved by the provided browsing service (HTTP 401), so no claim of newly verified upstream API behavior is made in this phase. M2A contains no OCCT, VTK, Open3D, or proprietary implementation code.

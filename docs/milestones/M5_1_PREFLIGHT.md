# Milestone 5.1 preflight and prerequisite gaps

The dependency-free repository configured, built, and passed all five registered tests on 2026-09-19. These cover M0 utilities and only the A-foundations of M1–M4. They are not a completed part/assembly product.

The premise diverges materially:

1. There is no production part feature history, dependency/recompute engine, stable TNaming resolver, solid/surface modeling, multibody, Save Bodies, or derived-body implementation.
2. There is no assembly domain, definition/revision/occurrence model, product tree, assembly persistence, display-asset cache, renderer backend, selection BVH, residency manager, scheduler, or session manager.
3. The existing Qt/OCCT viewport is an unverified single-box acceptance source; Qt 6 and OCCT 8 development packages remain absent from this runner.
4. M4A added discrete metadata and native OBJ/XYZ import, not the assumed VTK/Open3D display runtime.
5. Consequently there is no honest pre-M5 assembly baseline for viewport frame time, draw calls, GPU memory, cache hit rate, edit-in-context, tab switching, or network storage.
6. The supplied OCCT documentation pages returned HTTP 401 through the browsing service. No new OCCT API claims are encoded in M5.1A.

M5.1A therefore introduces measurement infrastructure and deterministic **corpus specifications**, then records only measurements supported by the current dependency-free runtime. Unsupported metrics are marked unavailable, never zero. It performs no assembly optimization and makes no renderer/storage decision.

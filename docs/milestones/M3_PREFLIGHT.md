# Milestone 3 preflight and prerequisite gaps

The dependency-free repository configured, built, and passed all three registered M0–M2 tests on 2026-09-19. Those tests cover core utilities, the M1A document aggregate with an in-memory store, and the M2A interaction framework. They are not a complete Milestone 0–2 product suite.

The Milestone 3 premise is not satisfied by the repository:

1. M1B–M1G are absent: there is no PlaneGCS integration, feature dependency/recompute engine, TNaming resolver, or production Pad/Pocket/Revolve/Fillet/Chamfer/Boolean implementation.
2. M2B–M2G are absent: there are no production solid features, multibody/Save Bodies, round-set executor, surface or mesh bodies, separated caches, or large-model suite.
3. The M2A feature framework has deterministic test doubles but no Qt schema renderer, OCAF schema persistence, or OCCT feature executor.
4. OCCT 8 and Qt 6 development packages are absent from this runner, so the existing conditional OCAF/desktop source still cannot be compiled here.
5. The public `GeometryShape` remains an ID placeholder rather than the opaque exact-geometry snapshot envisioned by ADR 0019.
6. The official OCCT pages supplied for M3 could not be retrieved by the available browsing service (HTTP 401). M3A deliberately contains no claims about newly verified OCCT API behavior and no kernel-specific conversion code.

M3A can safely establish kernel-neutral laws, continuity measurements, and deterministic complexity reports. It cannot honestly integrate those values into nonexistent advanced features or OCAF persistence. M3B must remain blocked until the missing M1/M2 geometry prerequisites and an OCCT-enabled build are available.

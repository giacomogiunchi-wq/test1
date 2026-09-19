# Milestone 4 preflight and divergence report

The dependency-free repository configured, built, and passed all four registered tests on 2026-09-19. They cover core utilities, the M1A in-memory document aggregate, M2A feature-session contracts, and M3A analytic law/quality utilities. They are not the complete M0–M3 system assumed by this milestone.

## Inventory result

There is no existing `MeshBody`, `PointCloudBody`, `IMeshKernel`, or discrete processing implementation. Only empty `src/cae/mesh` and `src/adapters/meshio` package markers existed. The public `Mesh` type in `interfaces.hpp` is a minimal CAE connectivity placeholder, not a CAD mesh body.

Consequently, the premise diverges as follows:

1. M1B–M1G and M2B–M2G remain absent, including production persistence, recompute, topological references, multibody, Save Bodies, surface/mesh bodies, and caches.
2. M3B–M3H remain absent, including advanced exact geometry and direct/derived features.
3. No mesh import/display path exists to benchmark before M4A. A "current mesh import/display" number would therefore be fabricated; the baseline is explicitly **not available**.
4. VTK, Open3D, PCL, Analysis Situs, OCCT 8, and Qt 6 development packages are not installed in this runner.
5. The official documentation links supplied for Open3D, VTK, PCL, and Analysis Situs returned HTTP 401 through the available browsing service. No unverified backend API is used in M4A.

M4A therefore implements the Duomec-owned discrete domain, non-destructive histories, adapter contracts, metadata persistence codec, and dependency-free OBJ/XYZ baseline importers. Open3D remains the intended production adapter and PCL/Analysis Situs remain disabled evaluations. M4B must not start before backend-enabled integration review.

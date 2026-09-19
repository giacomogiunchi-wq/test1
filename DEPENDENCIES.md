# Dependency and version plan

Versions are exact integration baselines, not floating minimums. Updates require an ADR, compatibility CI, and license review. System package managers may be used for developer builds only when they resolve to these versions; release builds will use locked recipes and checksums.

| Dependency | Pinned baseline | Role | Introduction |
|---|---:|---|---|
| CMake | 3.28.3 | build orchestration | M0 |
| Qt | 6.8.3 (LTS line) | desktop UI | M0 |
| Open CASCADE Technology | 8.0.0 | geometry, OCAF, XDE, AIS/V3d | M0–M2 |
| pybind11 | 2.13.6 | Python binding boundary | M0 scaffold |
| FreeCAD PlaneGCS | FreeCAD 1.0.2 source snapshot; exact commit to be vendored after extraction spike | sketch constraints | M1 |
| Gmsh | 4.14.1 | mesh generation | M3 |
| meshio | 5.3.5 | isolated Python format bridge | M3 |
| Code_Aster | 17.2 | structural solve | M3 |
| MEDCoupling/MED | 9.14.0 / 4.1.1 | MED interoperability | M3 |
| Project Chrono | 9.0.1 | multibody solve | M4 |
| OpenFOAM Foundation | 12 | CFD solve | M5 |
| VTK | 9.4.2 | embedded result visualization | M3 |
| ParaView | 5.13.3 | reference/advanced postprocessing | M3 |
| LAMMPS | 22 Jul 2025 stable | later GRANULAR adapter | post-MVP |
| Open3D | 0.19.0 candidate; revalidate before enabling | primary mesh/scan processing adapter | M4, disabled |
| PCL | 1.15.1 candidate; component audit required | optional scan segmentation adapter | M4, disabled |
| Analysis Situs open core | exact commit pending license/API spike | optional B-Rep recognizer reference/adapter | M4, disabled |

PlaneGCS is deliberately not treated as a stable library API. The M1 spike must select and record a FreeCAD commit, extract the smallest buildable source set, and test numerical behavior before its option can be enabled. Solver executables are runtime integrations: adapters must detect versions and reject incompatible majors.

Source URLs and license families are recorded in `THIRD_PARTY_LICENSES.md`. Upstream release metadata must be revalidated when locked build recipes are added; the bootstrap environment had no external network access.

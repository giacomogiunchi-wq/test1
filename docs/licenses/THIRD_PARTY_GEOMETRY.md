# Third-party geometry dependency matrix

This inventory is architectural planning, not legal advice. Exact notices and source archives must accompany every distributed binary. Versions marked "candidate" require upstream/API/license revalidation before enabling because the supplied browsing service was unavailable.

| Dependency | Baseline | License | Source | Link plan | Duomec modifications | Distribution obligations | Default |
|---|---|---|---|---|---|---|---|
| OCCT | 8.0.0 | LGPL-2.1 with OCCT exception | https://dev.opencascade.org/ | shared where supported | adapter only | license, exception and notices | Existing optional target |
| VTK | 9.4.2 | BSD-3-Clause | https://vtk.org/ | shared | adapter only | copyright/license notice | Disabled in M4A |
| Open3D | 0.19.0 candidate | MIT | https://github.com/isl-org/Open3D | shared | adapter only | copyright/license notice | Disabled pending recipe |
| PCL | 1.15.1 candidate | BSD-3-Clause (component audit required) | https://github.com/PointCloudLibrary/pcl | shared plugin | adapter only | component notices | Optional/disabled |
| Analysis Situs open core | exact commit TBD | repository/component license audit required | https://github.com/txemendes/AnalysisSitus | separate optional adapter | none planned | include all applicable notices; exclude commercial components | Optional/disabled |
| CGAL | no baseline | package-specific GPL/commercial combinations | https://www.cgal.org/ | none | none | package-level legal approval required before use | Excluded |

M4A adds **no new linked third-party dependency**. Its native OBJ/XYZ readers use only the C++ standard library so domain and persistence contracts can be tested before production adapter recipes are approved.

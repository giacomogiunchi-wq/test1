# Highest technical risks

| Risk | Why it matters | Mitigation / exit criterion |
|---|---|---|
| OCAF adoption | Transactions, labels, drivers, persistence and recompute semantics are difficult to retrofit. | M1 spike creates/edit/saves/reloads a two-feature document with undo and deterministic recompute before broad features. |
| PlaneGCS extraction | PlaneGCS has FreeCAD coupling and no promised standalone ABI. | Pin one commit, inventory transitive code/license, wrap behind `ISketchSolver`, and compare golden constrained sketches plus failure modes. Keep replacement possible. |
| Topological naming | Boolean and fillet edits can invalidate downstream face identity. | Combine TNaming with signatures/adjacency; test ambiguous edits and surface confidence/diagnostics instead of silently rebinding. |
| Windows/Linux solver deployment | Code_Aster/OpenFOAM are Linux-centric; paths, signals, containers, MPI and packaging differ. | Support native Linux first; specify tested container/WSL remote runner for Windows; one process contract and capability probe. |
| MED/Code_Aster interoperability | MED versions, groups, element ordering, names and units can silently diverge. | Golden round trips through Gmsh/MED/Code_Aster, validate group counts/orientation/units, pin MED stack. |
| Qt/OCCT event integration | Native windows, HiDPI coordinates, redraw lifecycle and gestures differ by platform. | Keep interaction in one viewport bridge; automated smoke tests plus Windows/Linux GPU and software-render CI. |
| Licensing boundaries | GPL/LGPL combinations and redistribution may constrain installers/plugins. | Maintain SBOM/notices, dynamically link LGPL components where applicable, isolate process tools, and require legal review per release. |
| OCCT 8 ecosystem maturity | API/package discovery may lag a new major version. | Compile adapters against only the pinned major and own compatibility shims; maintain one controlled dependency image. |
| Determinism and cancellation | External solvers and parallel meshers can leave corrupt state or children. | Content-addressed workdirs, manifests, fixed seeds/thread settings where supported, process-tree cancellation and atomic result promotion. |
| Units and result semantics | CAD commonly presents mm while solvers assume SI; stress/result location may be misread. | SI-only domain models, dimensional types/metadata, boundary conversion tests, explicit nodal/cell association. |

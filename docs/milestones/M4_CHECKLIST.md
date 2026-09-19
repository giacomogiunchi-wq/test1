# Milestone 4 implementation checklist

## M4A — Discrete geometry and scans (this change)

- [x] Distinct immutable mesh and point-cloud snapshots and first-class body metadata.
- [x] Mesh/point-cloud statistics, revisions, transforms, asset references, and provenance.
- [x] Non-destructive, editable, suppressible, cache-aware processing histories.
- [x] `IMeshKernel` and `IPointCloudKernel` ports with cancellation/progress contracts.
- [x] Dependency-free OBJ and XYZ baseline import adapters for contract testing.
- [x] Canonical metadata/history codec that never inlines raw geometry.
- [x] Unit tests, generated datasets, downstream-only invalidation tests, and benchmark.
- [x] Geometry dependency/license matrix.

## M4B — Repair, reduce, remesh
- [ ] Quantified diagnostics, conservative repair, sharp-edge-aware reduction/remeshing.

## M4C — Scan preparation and registration
- [ ] Crop/downsample/outliers/normals, ICP quality, and datum alignment.

## M4D — Segmentation and fitting
- [ ] Regions plus plane/cylinder/sphere/cone fits with explicit residuals.

## M4E — Assisted B-Rep reconstruction
- [ ] Analytic/freeform faces, trim/sew/validate, and source deviation maps.

## M4F — Dumb-solid recognition
- [ ] Duomec AAG and evidence-based hole/pocket/boss/chamfer/fillet candidates.

## M4G — Parametric reconstruction
- [ ] User-approved native feature rebuilding and geometric comparison.

## M4H — Sections and sketch extraction
- [ ] Sections, curve fitting, constraint suggestions, and PlaneGCS workflow.

## M4I — Large-data architecture
- [ ] Streaming/out-of-core adapters, LODs, external assets, and large-data gates.

Stop after M4A. Unchecked phases are not partially implemented.

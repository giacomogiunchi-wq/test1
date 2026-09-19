# Milestone 3 implementation checklist

## M3A — Geometry quality and laws (this change)

- [x] Kernel-neutral scalar-law interface and deterministic station validation.
- [x] Piecewise-linear and cubic interpolated laws.
- [x] Radius, scale, and twist law semantic wrappers.
- [x] Canonical versioned serialization and strict deserialization.
- [x] Requested continuity intent separated from measured quality.
- [x] Sampled gap, angular, and curvature deviation evaluator.
- [x] Deterministic curve/surface complexity and memory estimates.
- [x] Unit tests, analytic continuity cases, round trips, and microbenchmark.

## M3B — Advanced rounds

- [ ] Round sets, variable/partial round execution, OCCT diagnostics, and fallback strategies.

## M3C — Sweep and loft

- [ ] Orientation laws, guides, section correspondence, and twist inspection.

## M3D — Surface modeling

- [ ] Boundary conditions, fill/trim/extend/sew/thicken/replace operations.

## M3E — Surface quality tools

- [ ] Zebra, curvature, comb, normal, gap, and continuity overlays.

## M3F — Direct editing

- [ ] Conservative parametric face move/offset/replace/delete operations.

## M3G — Derived bodies

- [ ] Associative provenance, stale/update state, missing-source behavior, cycle detection.

## M3H — Complexity and file weight

- [ ] Advanced-feature instrumentation, preservation policies, and regression corpus.

Stop after M3A. No unchecked phase has an implementation in this change.

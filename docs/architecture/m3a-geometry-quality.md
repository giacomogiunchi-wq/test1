# M3A geometry laws and quality measurement

## Laws

`ScalarLaw` is immutable and uses a normalized independent parameter in `[0,1]`. A valid law has at least two finite, strictly ordered stations and explicitly contains endpoints 0 and 1. Evaluation outside the interval clamps to its nearest endpoint. `PiecewiseLinearLaw` is the predictable default. `InterpolatedLaw` uses monotonicity-preserving piecewise cubic Hermite slopes so a positive radius or scale cannot overshoot through zero between positive stations.

Canonical serialization uses `max_digits10` decimal values and a mandatory format/version prefix. Parsing is locale-independent and rejects trailing text, non-finite numbers, count mismatches, unknown versions, unordered stations, and unknown law kinds. Semantic wrappers distinguish SI radius, dimensionless scale, and radian twist even when their scalar representation is shared. An `IScalarLawAdapter` converts these domain objects to an opaque backend law; a future OCCT implementation privately owns `Handle(Law_Function)`.

## Continuity

Requested intent (`position`, `tangent`, or `curvature`) is never inferred from the measured result. `ContinuityEvaluator` samples two immutable `IBoundaryEvaluator` instances at the same normalized parameters, whose correspondence is an adapter responsibility. It reports maximum, mean, RMS, valid sample count, and normalized maximum-error location for:

* positional gap in metres;
* tangent-plane angular deviation in radians;
* curvature deviation in inverse metres.

Normals are compared when both evaluators provide normals; otherwise tangents are compared when both provide tangents. Directions are treated as unoriented for geometric continuity, so parallel and anti-parallel vectors have zero deviation. Missing data produces `unavailable` for requested continuity rather than an assumed pass. Fixed sample count is deterministic; adaptive refinement is deferred behind the same report shape.

## Complexity

Curve and surface reports record degrees, pole/knot/span counts, rational and periodic flags, bounding boxes, and requested/achieved/maximum entity tolerance. Approximate payload bytes count control-point coordinates (plus rational weight when present), knot doubles, and knot multiplicities. They deliberately exclude allocator, object, compression, OCAF, and file-container overhead, so the value is a deterministic relative-complexity metric rather than predicted file size.

## Limits

M3A does not construct geometry. The repository has no OCCT snapshot adapter from which to extract real B-Spline metadata and no M1/M2 feature persistence capable of storing these reports. Analytic unit-test evaluators verify the domain algorithms; kernel extraction, persistence, and feature integration are prerequisites for M3B and later phases.

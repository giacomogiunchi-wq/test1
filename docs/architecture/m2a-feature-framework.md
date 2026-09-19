# M2A unified feature framework

## Shared lifecycle

1. The application creates a `FeaturePreviewSession` from an immutable `FeatureDefinition` and, for editing, the existing `FeatureId`.
2. A schema renderer presents ordered common and advanced groups. Keyboard values are stored as typed domain values; manipulators update those same keys rather than maintaining parallel state.
3. Preselection is offered to collectors in definition order and consumed once by the first compatible collector. Collector cardinality, topology kind, duplication, and resolution state are validated before execution.
4. Preview validates first, cancels the preceding token, and calls `IFeatureExecutor` with `ExecutionQuality::preview`. Its shapes are transient.
5. Cancel requests cooperative stop, discards the transient result, and never calls `IFeatureTransaction`.
6. Commit validates again and calls `IFeatureTransaction` once with `ExecutionQuality::final`. The transaction implementation must begin, execute, accept the result, persist, and commit as one unit, or abort on any non-accepted result/error.

The session retains typed values and selections after validation or execution failure so UI callers can show diagnostics and let the user correct the feature. A successful commit closes the session. Creation and editing use the same definition and context; `edited_feature` is the only lifecycle distinction.

## Numeric policy

M2A validates finite scalar values and schema ranges in SI. A direction is valid when its squared magnitude is finite and greater than `1e-24`, corresponding to a magnitude above `1e-12`. This is a representation guard against a numerically zero direction, not an OCCT modeling tolerance. Feature-specific construction and OCCT fuzzy tolerances belong to M2B adapters and must be documented there.

## Threading and cancellation

The session is application-thread confined. `std::stop_source`/`std::stop_token` provides cooperative cancellation to an executor; adapters must poll at safe boundaries and must not publish a preview after cancellation. Background execution and UI dispatch are application-service responsibilities, keeping Qt out of the domain library.

## Deliberate M2A limits

The transaction and executor ports are tested with deterministic doubles because the repository lacks the assumed M1 feature/recompute services. No schema is persisted by the current M1A OCAF projection, and no Qt schema renderer or OCCT executor is claimed. These are prerequisite integration tasks, not hidden fallback behavior.

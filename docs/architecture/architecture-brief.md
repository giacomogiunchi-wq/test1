# Architecture brief

## Scope and layering

Milestone 0 establishes `core`, public ports, package boundaries, tests, and the Qt/AIS acceptance shell. Milestones 1–5 remain out of scope. The inner domain owns identifiers, SI-valued data, geometry handles, meshes, studies, constraints, and result fields. Adapter packages translate at their boundary; solver-native objects never leak into domain interfaces.

A composition root constructs adapters and injects their interfaces into application services. Commands own mutations and undo behavior. Qt classes render state and dispatch commands; feature and solver logic cannot live in UI classes.

## Packages

* `src/core`: errors/results, logging, commands, units, IDs, process policy.
* `src/cad/*`: geometry, features, sketches, topology, exchange, diagnostics, and later recognition.
* `src/cae/*`: solver-independent studies and mesh/result data.
* `src/adapters/*`: the only packages allowed to include an engine SDK or emit solver input.
* `src/ui/qt` and `src/scripting/python`: delivery mechanisms.
* `tests/unit`, `tests/integration`, `tests/golden_models`: fast contracts, installed-engine tests, and reviewed deterministic fixtures.

## External process contract

Every process adapter will use one shared runner with an argv vector (never a shell string), sanitized environment, deterministic directory `<workspace>/<study-id>/<input-hash>`, manifest, captured structured stdout/stderr events, startup/version probe, wall and idle timeouts, cooperative cancellation followed by bounded termination, and recorded exit status. A failed or cancelled run returns a structured error and retains its manifest/log for diagnosis.

## Geometry and persistence

OCAF is the transaction, persistence, and dependency backbone rather than a wrapper added later. Feature drivers consume typed parameters and references, return `FeatureResult`, and update a directed dependency graph. A Duomec reference combines TNaming history, geometry signature, adjacency context, and explicit confidence-ranked fallback; persistent face indices are prohibited.

## Test strategy

Core tests have no heavyweight dependencies. Each adapter gets contract tests and version-gated integration tests. Golden BREP/STEP/study assets are small and reviewed. Numerical validations state units, tolerances, reference derivations, and engine versions. GUI acceptance includes native event smoke tests on Linux and Windows once dependency images exist.

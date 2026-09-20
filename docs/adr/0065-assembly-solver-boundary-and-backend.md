# ADR 0065: Assembly solver boundary and initial backend

* Status: Accepted with backend limitation
* Date: 2026-09-19

## Context

M5.2 requires a replaceable solver and asks for OndselSolver evaluation. This runner cannot access the official repository (GitHub CONNECT returned 403 and web documentation returned 401), and no Ondsel package/source is present. Its license, current version, incremental behavior, loops, conflict/redundancy behavior, and warm-start API therefore cannot be verified responsibly.

## Decision

`IAssemblyConstraintSolver` is the only domain-facing solver contract. Step 5.2.2 supplies `NativeAssemblyConstraintSolver` version 0.1, an in-tree GPL-3.0 semantic/DOF and consistency baseline. It consumes only transforms, local frames, geometry descriptors, and parameters. It detects underconstraint, duplicate redundancy, and conflicting duplicate semantics, but is not represented as an Ondsel adapter or a general nonlinear geometric solver.

## Consequences

No unverified dependency or third-party type enters the domain. An `OndselAssemblySolverAdapter` may be added only after a reproducible source/version/license/API evaluation. General closed-loop nonlinear convergence remains an explicit risk rather than a false capability claim.

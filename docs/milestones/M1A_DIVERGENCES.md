# Milestone 1A preflight and divergences

The Milestone 0 core configured, built, and passed its existing test on 2026-09-19. The dependency-equipped desktop target could not be verified in this runner because Qt 6 and OCCT development packages are absent.

Repository divergences found before 1A:

1. `IParametricDocument` exposed only `recompute` and `save`; it had no domain aggregate or load/transaction contract.
2. `EntityId` was an unvalidated string alias, so persistent entity kinds could be mixed accidentally.
3. The original `Result<T>` required `T` to be default constructible and permitted reading the inactive side.
4. There was no OCAF target, application, schema, persistence adapter, binary integration test, or transaction stress test.
5. Milestone 0's command stack was in-memory only and could not coordinate an OCAF transaction.
6. The desktop source existed but had not been compiled in the provided environment; its acceptance behavior therefore remains unverified rather than assumed.
7. Dependency versions were documented but no locked dependency acquisition recipe or checksums existed. OCCT 8 remains an externally supplied requirement.

Milestone 1A addresses items 1–5 within its scope. Items 6–7 remain environment/release-engineering risks and are not bypassed with mocks presented as OCAF tests.

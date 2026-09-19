# Step 5.2.1 preflight and M5.1 prerequisite audit

The repository passes its dependency-free M0–M5.1A suite, but it does **not** contain the assumed completed M5.1 runtime. In particular, there is no definition/revision/occurrence graph, authoritative assembly container, asset cache, residency manager, scheduler, renderer backend, spatial index, selection runtime, session manager, or edit-in-context path. `docs/milestones/M5_1_CHECKLIST.md` correctly leaves M5.1B–J incomplete.

Step 5.2.1 therefore adds only a dependency-light semantic metadata library. It does not manufacture a second occurrence graph or pretend the absent runtime exists. Its `OccurrenceState` is the persisted mobility/insertion payload that a future M5.1B occurrence record must embed or reference. No API traverses B-Rep, tessellates, performs a global recompute, or loads authoring state.

Required prerequisite extensions in this step are limited to persistent definition, occurrence, relation, endpoint, frame, and solve-island IDs plus lightweight part/assembly absolute-reference metadata. Integration into a chunked authoritative assembly document and undoable assembly aggregate remains blocked on M5.1B/C and must not be improvised here.

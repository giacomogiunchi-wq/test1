# M5.1A bottlenecks

1. **No assembly runtime exists.** The repository cannot yet measure occurrence graph construction or transform-only interaction.
2. **No renderer benchmark is possible in this runner.** Qt, OCCT and a graphics context are absent; the OCCT-versus-instancing decision remains explicitly open.
3. **Trace export is intentionally simple.** It snapshots and sorts all events and emits one in-memory JSON string. Streaming and bounded retention are future hardening work.
4. **Recorder contention is visible in tail latency.** A mutex protects event ordering and ownership; M5.1D must benchmark per-thread buffering before changing this correctness-first design.
5. **The corpus is a deterministic specification, not synthetic CAD data.** Geometry, presentation, source I/O and network simulation must be layered on after the occurrence model exists.

No optimization or custom renderer was introduced in M5.1A.

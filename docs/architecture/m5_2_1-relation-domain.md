# M5.2.1 relation-domain dependencies

```text
assembly relation semantics
  ├── persistent IDs ───────────────> CadDocument identity utility
  ├── lightweight definition refs   (no authoring/B-Rep dependency)
  ├── occurrence mobility/transform (no renderer dependency)
  ├── compact geometry + frames     (no OCCT dependency)
  ├── DOF result vocabulary         (no solver dependency)
  └── motion/FEM hints              (no Chrono/FEM dependency)

future adapters (not implemented)
  solver / Chrono / FEM / persistence container
       depend on semantic domain, never the reverse
```

Fixed placement is an occurrence state and creates no relation. Rigid/Flexible controls future solve-graph condensation independently of Fixed/Floating. Absolute references live in definition metadata so availability never promotes a definition to exact-geometry or authoring residency.

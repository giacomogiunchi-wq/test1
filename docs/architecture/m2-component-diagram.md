# Milestone 2 dependency diagram

```mermaid
flowchart TB
  QT[Qt feature panel renderer] --> APP[Application commands / transaction coordinator]
  APP --> SESSION[FeaturePreviewSession]
  SESSION --> DEF[FeatureDefinition + schemas]
  SESSION --> EXEC[IFeatureExecutor port]
  SESSION --> TX[IFeatureTransaction port]
  DEF --> DOMAIN[Duomec CAD domain]
  TX --> DOC[Parametric document port]

  EXEC --> OCCTAD[OCCT geometry adapter]
  DOC --> OCAFAD[OCAF document adapter]
  MESH[IMeshKernel port] --> VTKAD[VTK adapter]
  MESH --> O3DAD[optional future Open3D adapter]

  OCCTAD --> OCCT[(OCCT 8.x)]
  OCAFAD --> OCAF[(OCAF)]
  VTKAD --> VTK[(VTK)]
  O3DAD --> O3D[(Open3D)]
```

Arrows point from consumer to dependency. UI renders immutable definitions and submits domain values; it cannot execute feature semantics. Domain headers include no Qt, OCCT, OCAF, VTK, or Open3D types. M2A implements the definition, collector, preview-session, executor, and transaction ports only. Adapter boxes beyond the existing OCAF work are future phases.

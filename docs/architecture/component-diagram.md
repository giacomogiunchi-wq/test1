# Component diagram

```mermaid
flowchart TB
  UI[Qt 6 UI] --> APP[Application services / commands]
  PY[pybind11 scripting] --> APP
  APP --> DOMAIN[Duomec CAD + CAE domain models]
  APP --> PORTS[Stable Duomec interfaces]
  DOMAIN --> CORE[Core: Result/Error, units, logging, IDs]
  PORTS --> CORE
  subgraph Adapter packages
    OCCT[OCCT geometry + AIS]
    OCAF[OCAF document]
    PG[PlaneGCS]
    IO[XDE STEP]
    GM[Gmsh / meshio]
    AS[Code_Aster process]
    OF[OpenFOAM process]
    CH[Chrono]
    VP[VTK post]
    DEM[LAMMPS, later]
  end
  PORTS --> OCCT & OCAF & PG & IO & GM & AS & OF & CH & VP & DEM
  OCCT --> K[(OCCT 8)]
  OCAF --> K
  IO --> K
  AS --> A[(Code_Aster)]
  OF --> F[(OpenFOAM)]
```

Dependencies point inward: UI and adapters may depend on interfaces/domain, never the reverse. A subsystem cannot bypass its port to call a solver. Cross-adapter exchange uses Duomec models or versioned files in deterministic job directories.

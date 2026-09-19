# Milestone 4 discrete-geometry dependency diagram

```mermaid
flowchart TB
  UI[Qt workspace] --> APP[Application commands]
  APP --> HIST[Mesh / point-cloud histories]
  HIST --> DOMAIN[Duomec discrete domain]
  APP --> MK[IMeshKernel]
  APP --> PK[IPointCloudKernel]
  DOMAIN --> ASSET[External asset reference + revision]

  MK --> NATIVE[Native OBJ baseline adapter]
  PK --> NATIVEPC[Native XYZ baseline adapter]
  MK --> O3DM[Future Open3D mesh adapter]
  PK --> O3DP[Future Open3D point adapter]
  PK --> PCL[Optional PCL adapter]
  REC[Future IBRepRecognizer] --> NREC[Duomec native recognizer]
  REC --> ASIT[Optional Analysis Situs adapter]
  RECON[Future IReconstructionKernel] --> OCCT[OCCT adapter]
  DISPLAY[Display adapter] --> VTK[VTK]
```

Domain and history targets contain no Qt, Open3D, VTK, PCL, Analysis Situs, or OCCT types. Backends consume immutable Duomec snapshots and publish new revisions. The native OBJ/XYZ adapters are deliberately narrow bootstrap implementations, not substitutes for the reviewed Open3D production adapter.

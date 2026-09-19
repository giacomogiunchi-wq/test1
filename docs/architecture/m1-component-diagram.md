# Milestone 1 component diagram

```mermaid
classDiagram
  class ParametricDocument
  class Body
  class Feature
  class Parameter
  class IDocumentStore
  class OcafDocumentStore
  class DependencyGraph
  class RecomputeEngine
  class PlaneGCSAdapter
  class OcctGeometryAdapter
  ParametricDocument *-- Body
  Body *-- Feature
  Feature *-- Parameter
  ParametricDocument --> IDocumentStore
  IDocumentStore <|.. OcafDocumentStore
  OcafDocumentStore --> OCAF
  RecomputeEngine --> DependencyGraph
  PlaneGCSAdapter --> PlaneGCS
  OcctGeometryAdapter --> OCCT
```

Only the left-hand domain objects and `IDocumentStore` are visible to application commands. The three adapters contain their respective external headers. Milestone 1A implements the document aggregate and OCAF store only; the remaining classes are sequenced in 1B–1F.

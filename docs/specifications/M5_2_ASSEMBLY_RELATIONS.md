# Duomec Platform — Codex Prompt for Milestone 5.2 — Revision 2
## Assembly Relations, Quick Mates, Component Lifecycle, Constraint Solving and Motion/CAE-Ready Semantics

### Mission

You are the lead C++ CAD/kinematics architect implementing **Milestone 5.2 — Assembly Relations, Quick Mates, Constraint Solving and Motion-Ready Kinematics** for the Duomec Platform.

Milestone 5.1 is assumed complete and provides:

- `PartDefinition` / immutable `PartRevision` / `AssemblyOccurrence`;
- large-assembly runtime graph;
- definition-level geometry/display sharing;
- content-addressed caches;
- progressive/lazy loading;
- explicit HOT/WARM/COLD residency;
- exact geometry and authoring loaded only on demand;
- assembly BVH and hierarchical selection;
- local detailed face/edge/vertex selection;
- transform-only component movement path;
- render backend abstraction;
- CPU/GPU scheduling and performance instrumentation;
- assembly invalidation domains;
- revision-safe background tasks.

Milestone 5.2 adds **assembly semantics** on top of that runtime.

The critical product goals are:

1. Support the full practical mate/assembly-relation vocabulary expected by a professional SOLIDWORKS user.
2. Make applying mates faster and less modal than traditional property-panel workflows.
3. Permit mates to component/assembly absolute origins, primary planes, axes and coordinate frames without loading full B-Rep.
4. Support professional occurrence lifecycle operations: Fix/Float, Rigid/Flexible behavior, Replace Component, Virtual Component, Create Subassembly and Make Independent.
5. Preserve smooth direct manipulation for free and partially constrained components.
6. Solve only the affected assembly subgraph.
7. Keep the relation solver independent from B-Rep during ordinary solve/drag operations.
8. Preserve semantic information so every assembly relation can later become a clean kinematic constraint/joint in the Duomec multibody environment.
9. Preserve relation references so they can later be used as **analysis hints** for FEM contact/connector/boundary-condition setup without carrying FEM runtime overhead in the assembly environment.
10. Prepare for a future multibody workflow comparable in intent to SOLIDWORKS Motion, while keeping dynamics and FEM execution outside this milestone.

---

# 1. Core product principle

An assembly mate is not merely a command that moves geometry into place.

It is a persistent engineering relationship with three simultaneous meanings:

```text
GEOMETRIC INTENT
    how selected entities relate

KINEMATIC INTENT
    which relative degrees of freedom remain

MOTION / ANALYSIS INTENT
    how the relationship should later become a multibody joint/constraint
```

Therefore Duomec must never represent all relations only as anonymous solver equations.

The **Duomec relation model is authoritative**.

The numerical assembly solver and future Project Chrono multibody backend are adapters.

---

# 2. Future multibody requirement

The later Duomec Motion/MBD environment is planned around **Project Chrono**.

Project Chrono is not the authoring-domain model for Milestone 5.2.

Instead define:

```text
Duomec AssemblyRelation
        ↓
Duomec Kinematic Semantics
        ├── design-time constraint solver
        └── future Project Chrono exporter/compiler
```

A mate created today must carry enough semantic information to be translated later into:

- fixed relation;
- revolute joint;
- prismatic joint;
- cylindrical joint;
- spherical joint;
- universal joint;
- planar relation;
- point-line;
- point-plane;
- trajectory/path relation;
- gear relation;
- rack-pinion;
- screw/helical relation;
- pulley/belt relation;
- generic holonomic constraint where no higher-level joint exists.

Do not lose this intent by flattening every relation permanently into independent scalar equations.

---

# 3. Solver technology policy

## Duomec-owned API

Create and keep stable:

```cpp
class IAssemblyConstraintSolver;
class IAssemblyConstraintCompiler;
class IConstraintSolveSession;
```

No third-party solver types may leak into the domain layer.

## Primary open-source design-time solver candidate

Evaluate the **FreeCAD-maintained OndselSolver** as the first backend.

Reasons:
- specifically designed for assembly constraints;
- supports 3D assembly solving;
- originates from assembly/multibody work;
- used by FreeCAD's built-in Assembly workbench;
- open source under LGPL-2.1;
- FreeCAD currently carries `FreeCAD/OndselSolver` as a dependency/submodule.

Create:

```cpp
class OndselAssemblySolverAdapter final
    : public IAssemblyConstraintSolver;
```

### Mandatory evaluation gate

Do not blindly couple the architecture to OndselSolver.

Before committing it as the sole production backend, benchmark:

- supported primitive constraints;
- closed loops;
- redundant constraints;
- large solve islands;
- poor initial conditions;
- incremental/warm-start solving;
- underconstrained mechanisms;
- overconstrained/conflicting systems;
- repeated interactive drag;
- numerical stability.

If the backend is insufficient for a specific higher-level mate, implement that mate as a Duomec semantic composite compiled into supported primitives, or keep an alternate backend path open.

The domain model must allow replacing the solver later.

---

# 4. Relation model

Create persistent IDs:

```cpp
using AssemblyRelationId = Uuid;
using RelationEndpointId = Uuid;
using KinematicFrameId   = Uuid;
using SolveIslandId      = Uuid;
```

Core model:

```cpp
class AssemblyRelation {
public:
    AssemblyRelationId id;
    AssemblyRelationType type;

    std::vector<RelationEndpoint> endpoints;
    RelationParameters parameters;

    RelationState state;
    RelationSolveMetadata solveMetadata;
    MotionSemanticDescriptor motionSemantics;

    bool suppressed;
};
```

Each endpoint:

```cpp
struct RelationEndpoint {
    RelationEndpointId id;
    OccurrenceId occurrence;

    TopologyReference topologyReference;

    LocalKinematicFrame localFrame;
    GeometryDescriptor geometryDescriptor;

    ReferenceResolutionState referenceState;
};
```

---

# 5. Critical performance rule: compile geometry once

Normal constraint solving must **not require traversing B-Rep geometry on every solve**.

At mate creation or reference re-resolution:

```text
selected B-Rep/reference geometry
        ↓
extract compact local geometric descriptor
        ↓
LocalKinematicFrame + scalar parameters
        ↓
persistent RelationEndpoint
```

Examples:

Planar face:
```text
local origin
local normal
plane offset
```

Cylindrical face:
```text
axis origin
axis direction
radius
```

Circular edge:
```text
center
normal
radius
```

Linear edge / axis:
```text
origin
direction
```

Vertex:
```text
local point
```

Coordinate system:
```text
local rigid frame
```

Spline/path:
```text
persistent path reference
compact curve representation / evaluation adapter
```

During normal solve:

```text
Occurrence transforms
+ compact endpoint frames/descriptors
+ relation parameters
```

must be sufficient.

Only reload exact geometry when:
- creating a new geometry-based relation;
- repairing/re-resolving a changed reference;
- a specialized relation truly needs curve/surface evaluation;
- the part revision invalidates the stored descriptor.

This requirement is essential for large assemblies.

---

# 6. Geometry descriptor

Create a solver-neutral descriptor:

```cpp
enum class GeometryClass {
    Point,
    Line,
    Axis,
    Plane,
    Circle,
    Cylinder,
    Cone,
    Sphere,
    CoordinateFrame,
    Curve,
    Path,
    Surface,
    Profile,
    Slot,
    CamProfile,
    Unknown
};

struct GeometryDescriptor {
    GeometryClass type;
    LocalKinematicFrame frame;

    optional<double> radius;
    optional<double> coneHalfAngle;

    GeometryProperties properties;
};
```

`TopologyReference` remains authoritative for reattachment.

`GeometryDescriptor` is the cached compact kinematic interpretation.

---

# 7. Local kinematic frames

Every relation must define explicit frames.

```cpp
struct LocalKinematicFrame {
    Vec3 origin;
    Quaternion orientation;
};
```

Frame axes convention must be documented globally.

Recommended semantic convention:

```text
Z = principal mate/joint axis
X = primary orientation/reference direction
Y = Z × X
```

For planar relations:
- Z = face normal.

For cylindrical/concentric relations:
- Z = cylinder/axis direction.

For circular edge:
- origin = center;
- Z = circle normal.

For point:
- origin = point;
- orientation derived only when additional references exist.

Stable frames are required for:
- deterministic mate solve;
- avoiding angle flip;
- future reaction-force coordinate systems;
- future Project Chrono link/joint construction.

---

# 7A. Absolute reference geometry — always lightweight

Every part, subassembly and top-level assembly must expose a stable lightweight absolute-reference set.

Create:

```cpp
struct AbsoluteReferenceSet {
    PersistentReferenceId origin;
    PersistentReferenceId frontPlane;
    PersistentReferenceId topPlane;
    PersistentReferenceId rightPlane;

    PersistentReferenceId xAxis;
    PersistentReferenceId yAxis;
    PersistentReferenceId zAxis;

    LocalKinematicFrame absoluteFrame;
};
```

These references are **authoritative lightweight metadata**, not B-Rep faces.

They must be available in `Metadata` residency from Milestone 5.1.

Therefore the user can create mates involving:

- component origin ↔ assembly origin;
- component origin ↔ component origin;
- Front/Top/Right plane ↔ assembly or component plane;
- origin ↔ plane;
- axis ↔ axis;
- primary coordinate frame ↔ primary coordinate frame;

without loading part authoring history or exact body geometry.

### UI

Reference geometry can be exposed by:

- tree nodes under each component;
- contextual `Show Reference Geometry`;
- origin/plane/axis hover/display mode;
- Quick Mate selection.

### Frame mate

Selecting two origins/coordinate frames should offer a high-level `Frame Mate`.

`Frame Mate` may constrain:

- origin coincidence;
- axis alignment;
- full orientation;

in one semantic relation.

Do not represent the component's primary planes as ordinary modeled planar faces.

---

# 7B. Default insertion frame metadata

The Milestone 5.1 part manifest must always carry:

```text
Local Part Origin
Front Plane
Top Plane
Right Plane
X/Y/Z axes
```

This metadata is tiny and should never require exact geometry, mass properties or feature-history loading.

It is also used for:

- default insertion at assembly origin;
- component rotation pivot;
- origin/plane mates;
- future multibody joint frames;
- FEM reference-frame suggestions.

---

# 8. Degrees of freedom model

A free rigid occurrence has:

```text
Tx Ty Tz
Rx Ry Rz
```

Create:

```cpp
struct DofState {
    DofMask freeDofs;
    DofMask constrainedDofs;

    int independentDofCount;
};
```

Duomec must calculate and expose:
- component/solve-island DOF;
- fully constrained;
- underconstrained;
- grounded/fixed;
- redundant constraints;
- conflicting constraints.

Do not conflate:

```text
REDUNDANT
```

with:

```text
CONFLICTING
```

A redundant relation may be kinematically consistent but unnecessary.

A conflicting relation cannot be simultaneously satisfied.

This distinction is important for later multibody analysis because redundant constraints can create poor reaction-force results.

---

# 9. Relation taxonomy — full professional baseline

Implement the following relation families.

## 9.1 Standard relations

Required:

```text
Coincident
Concentric
Distance
Angle
Parallel
Perpendicular
Tangent
Lock
```

### Coincident

Support appropriate combinations of:
- point/point;
- point/plane;
- point/line/axis where meaningful;
- planar face/planar face;
- coordinate frames/origins where applicable;
- selected reference geometry.

### Concentric

Support:
- cylinder/cylinder;
- cylinder/axis;
- circular edge/circular edge;
- circular edge/axis;
- cone/coaxial compatible geometry where valid.

Options:
- aligned / anti-aligned axis;
- **Lock rotation**.

`Lock rotation` must be explicit because it changes kinematic DOF.

### Distance

Support compatible:
- planes;
- axes/lines;
- points;
- cylinders;
- spheres;
- curves where a deterministic distance definition exists.

Store:
- signed orientation/reference side;
- value;
- optional driven/reference state for future extension.

### Angle

Store an explicit angular reference frame to avoid solution flips.

Support optional reference entity / auto-selected stabilizing reference.

Do not rely only on `acos(dot(a,b))`, which loses orientation information.

### Tangent

Support expected analytic combinations where robust:
- plane/cylinder;
- plane/sphere;
- cylinder/cylinder;
- sphere/cylinder;
- supported surface cases.

Do not promise arbitrary freeform-surface tangent assembly constraints unless the solver/evaluator can maintain them reliably.

### Lock

Lock relative six-DOF pose between two occurrences or occurrence and assembly ground.

Do not deep-copy transforms.

---

# 10. Advanced relations

Required:

```text
Limit Distance
Limit Angle
Linear / Linear Coupler
Path
Profile Center
Symmetric
Width
```

## Limit Distance

Parameters:
- current/start value;
- min;
- max;
- direction/reference frame.

The relation is unilateral/conditional at limits.

Future multibody export must preserve limit semantics instead of converting it into a permanently fixed distance.

## Limit Angle

Same principle:
- min angle;
- max angle;
- stable angular frame;
- anti-flip reference.

## Linear / Linear Coupler

Relationship:

```text
s2 = ratio * s1 + offset
```

Support:
- user ratio;
- reversed direction;
- reference frame/ground;
- stable axis definitions.

## Path

Constrain a point/frame on an occurrence to a curve/path.

Required modes:

Position:
- Free along path;
- Distance along path;
- Percent along path.

Orientation:
- Free;
- Follow tangent;
- explicit pitch/yaw/roll policy where supported.

Store path parameterization independently from transient tessellation.

## Profile Center

Center-align compatible:
- circular profiles;
- rectangular profiles;
- regular polygonal profiles where recognition is reliable.

Options:
- alignment;
- angular orientation;
- offset;
- lock rotation.

## Symmetric

Two entities/components symmetric about:
- plane;
- planar face;
- assembly reference plane.

## Width

Support:
- width references;
- tab references.

Modes:
- Center;
- Free;
- Dimension/Distance from end;
- Percent.

Do not implement width mate merely as two coincident constraints; preserve semantic intent.

---

# 11. Mechanical relations

Required:

```text
Cam
Gear
Hinge
Rack and Pinion
Screw
Slot
Universal Joint
```

These must be first-class high-level relations because they are extremely important for the future multibody layer.

## Cam

Inputs:
- follower: point, cylindrical face/roller, supported planar reference;
- cam profile/path/surface.

Maintain:
- contact side;
- follower geometry;
- path/profile reference.

Design-time solve may use a specialized evaluator.

Future MBD mapping:
- contact/cam constraint, not a generic coincidence.

## Gear

Store:
- axis 1;
- axis 2;
- ratio;
- reverse direction;
- optional pitch diameters;
- phase/reference orientation when relevant.

Kinematic relation:

```text
theta2 = ratio * theta1 + phase
```

For external gears direction is normally opposite; do not infer physical tooth contact from visible teeth alone.

## Hinge

Represent as one semantic joint.

Result:
- five relative DOF constrained;
- one rotation remains.

Inputs:
- axis references;
- axial position references where necessary.

Optional:
- min/max angle hooks.

Future MBD:
- direct revolute-joint mapping.

## Rack and Pinion

Store:
- rack axis;
- pinion axis;
- pitch diameter or travel/revolution;
- ratio;
- direction;
- phase/offset.

Relation:

```text
s = r * theta + offset
```

## Screw

Store:
- common axis;
- lead/pitch;
- handedness/direction;
- phase.

Relation:

```text
s = lead/(2π) * theta + offset
```

Future MBD:
- screw/helical joint.

## Slot

Inputs:
- follower point/axis/cylinder;
- slot path.

Modes:
- Free;
- Center in Slot;
- Distance Along Slot;
- Percent Along Slot.

Orientation behavior must be explicit.

## Universal Joint

Store:
- input axis/frame;
- output axis/frame;
- joint center;
- phase/orientation references.

Future MBD:
- direct universal-joint mapping.

---

# 12. Additional SOLIDWORKS-like assembly relations/features to support

These are not all ordinary two-entity mates, but professional compatibility requires architectural support.

## Belt / Chain relation

Create:

```cpp
class BeltRelation;
```

Inputs:
- pulley/sprocket axes;
- effective pitch radii;
- wrap orientation;
- engaged/disengaged state.

Store:
- belt length;
- relation among rotations;
- optional belt-path representation.

Future MBD:
- pulley/belt relation.

Do not require actual tooth/chain geometry.

## In-Place relation

For components created in assembly context.

Store the intentional fixed contextual pose and external-reference semantics separately from a normal user-created Lock relation.

## Lock To Sketch / Path

Needed for generated assembly features such as belt parts and other construction workflows.

## Coordinate / Frame Mate — Duomec extension

Provide an explicit frame-to-frame relation:

```text
origin coincidence
+ axis alignment
+ optional orientation offset
```

This is especially valuable for:
- machinery;
- imported systems;
- robotics;
- multibody export.

It can coexist with compatibility behavior where selecting two coordinate systems suggests Coincident/Frame Mate.

## Magnetic / Connection-Point relation

Architectural support for:
- published connection point;
- ground plane;
- snap/auto-connect asset insertion.

This may be implemented after core mate families but remains within 5.2 scope if time allows.

---

# 13. Quick Mates — mandatory primary UX

The user must not need to open a Mate panel for the common workflow.

This is a critical Duomec requirement.

## Trigger

When the user:

```text
Ctrl + selects compatible geometry from two or more components
```

such as:
- faces;
- edges;
- vertices;
- axes;
- planes;
- origins;
- coordinate systems;

the system immediately computes compatible mate candidates.

Display a compact contextual toolbar **next to the cursor**, without leaving the graphics area.

---

# 14. Quick Mate toolbar behavior

Create:

```cpp
class MateCandidateEngine;
class QuickMateController;
class QuickMateOverlay;
```

Flow:

```text
Ctrl-select geometry
    ↓
GeometryDescriptor extraction
    ↓
MateCandidateEngine
    ↓
rank valid mate candidates
    ↓
show small cursor-adjacent toolbar
    ↓
live preview default candidate
    ↓
click / Enter to commit
```

Only geometrically valid/relevant mate buttons are shown.

The default/highest-confidence mate is visually highlighted.

Keyboard:
- `Enter` = accept highlighted mate;
- `Tab` = flip alignment where relevant;
- `Esc` = cancel toolbar/preview.

Do not force the user into a modal PropertyManager for normal mates.

---

# 15. Quick Mate candidate rules

Examples:

### two parallel planar faces

Primary suggestion:
```text
Coincident
```

Alternatives:
```text
Distance
Parallel
Angle
```

### two cylindrical faces / compatible circular edges

Primary:
```text
Concentric
```

Inline secondary:
```text
Lock Rotation
```

### cylinder/axis

Primary:
```text
Concentric
```

### plane + point/vertex

Primary:
```text
Coincident
```

Alternative:
```text
Distance
```

### planar faces currently perpendicular

Suggest:
```text
Perpendicular
Angle
```

### tangent-capable plane/cylinder or cylinder/cylinder

Suggest:
```text
Tangent
```

### two coordinate systems

Primary:
```text
Frame/Coordinate Mate
```

### compatible slot + cylindrical/axis reference

Primary:
```text
Slot
```

### two suitable profiles

Offer:
```text
Profile Center
```

---

# 16. Quick composite mates — improve on the basic workflow

Implement semantic composites for common mechanical tasks.

## Insert / Peg-in-Hole

If the user selects suitable circular edges/faces:

```text
shaft/hole axis
+
adjacent planar seating references
```

offer:

```text
Insert
```

Duomec `InsertRelation` is a high-level composite.

Internally it may compile to:
- Concentric
- Coincident

Options:
- free rotation;
- lock rotation.

For future MBD:
- free rotation maps naturally to a Revolute semantic;
- locked rotation maps to Fixed where no other DOF remains.

Do not lose the composite intent merely because the design-time solver uses two primitive equations.

## Hinge quick relation

When references imply a hinge, offer a direct Hinge button rather than making the user build multiple mates manually.

---

# 17. Inline parameter entry

Distance/Angle/Limit/etc. should be editable next to the toolbar.

Example:

```text
[Coincident] [Parallel] [Distance  12.00 mm] [↔ Flip]
```

For limits:

```text
Limit Distance
min  0 mm
max  35 mm
```

No large modal panel is required unless Advanced options are requested.

---

# 18. Full Mate panel still exists

Quick Mates do not replace a full inspector.

The standard relation editor is required for:
- advanced mates;
- mechanical mates;
- multiple parameters;
- diagnostics;
- reference replacement;
- multibody metadata;
- advanced orientation.

But the common workflow should remain graphics-area-first.

---

# 19. Smart insertion and Mate References

Support reusable mate-reference metadata on part definitions.

Create:

```cpp
class MateReferenceDefinition;
```

Each part can define:
- primary mate reference;
- secondary;
- tertiary.

Store:
- reference geometry;
- preferred mate type;
- alignment;
- local frame;
- priority/name.

During insertion:

```text
drag/insert part
 -> detect compatible target reference
 -> preview snapped pose
 -> show proposed mates
 -> drop
 -> commit
```

This is valuable for:
- fasteners;
- bearings;
- electrical/mechanical connectors;
- catalog components.

Do not require exact feature history for Mate References; store them as lightweight part metadata compatible with M5.1.

---

# 19A. Default component insertion — origin aligned and fixed

Implement a deterministic insertion mode for components added without interactive cursor placement.

If the user inserts/imports one or more parts/subassemblies and **does not specify a placement point/pose with the mouse**:

```text
Occurrence local transform = Identity relative to parent assembly frame
Component Origin = Parent Assembly Origin
Front Plane       = Parent Front Plane
Top Plane         = Parent Top Plane
Right Plane       = Parent Right Plane
PlacementMobility = Fixed
```

Important:

- this is **not** implemented by creating Coincident/Parallel mates;
- it is a fixed occurrence transform;
- the component has no artificial mate records;
- primary reference geometry remains available if the user later chooses to Float and mate it.

If multiple components are imported in the same no-placement operation, each is inserted origin-aligned and Fixed, even if they overlap geometrically.

If the user explicitly places a component with the mouse:

- preserve the chosen transform;
- default to `Floating`, unless the user chose a `Fix after placement` option.

For insertion into a subassembly, “assembly origin” means the current parent subassembly frame.

This behavior must not load exact B-Rep.

---

# 19B. Replace Component

Implement:

```cpp
class ReplaceComponentCommand;
class ReplacementReferenceMapper;
```

Support:

- replace one selected occurrence;
- replace multiple selected occurrences;
- replace all occurrences of a selected definition when explicitly requested;
- part → part;
- part → subassembly;
- subassembly → part;
- subassembly → subassembly.

### Runtime behavior

Keep the existing `OccurrenceId` where possible.

Replace:

```text
PartDefinitionRef
```

while preserving:

- current world/local assembly transform;
- occurrence metadata;
- visibility/suppression;
- applicable configuration state;
- user appearance overrides.

Do not reload/recompute unrelated assembly definitions.

### Mate/reference migration priority

Try reattachment in this order:

1. explicit Mate Reference / published connection reference IDs;
2. absolute origin/Front/Top/Right/axis references;
3. stable named topology references;
4. compatible semantic geometry descriptors;
5. geometric/topological signature matching;
6. user-guided reference replacement.

For every existing relation:

```text
Resolved
NeedsReview
DanglingReference
Incompatible
```

Never silently delete unresolved relations.

Never silently attach a relation to an arbitrary “closest” face.

Provide a replacement preview.

The user must be able to inspect/reassign failed references before final commit or accept a commit with explicit dangling relations.

### Cache behavior

If replacement definition/revision already exists in M5.1 cache:
- reuse its display/exact assets.

The old definition remains untouched for other occurrences.

---

# 19C. Virtual Components — embedded part or assembly

Implement virtual components as first-class Duomec definitions stored inside the parent assembly's authoritative container.

Support:

```text
Virtual Part
Virtual Subassembly
```

Create:

```cpp
struct EmbeddedComponentDefinition {
    PartDefinitionId definitionId;
    EmbeddedComponentType type;
    ParentAssemblyId owner;
};
```

A virtual component:

- has its own stable definition/revision identity;
- can have feature history;
- can be edited in assembly context;
- can own bodies/material/metadata;
- can have mates like any external component;
- may reference assembly geometry through controlled external/context references;
- does not require an external `.duomecpart` / `.duomecasm` file.

### Create in context

Commands:

```text
New Virtual Part
New Virtual Subassembly
```

The user may create the component:
- at the parent assembly origin;
- on a selected plane;
- using a selected frame/reference.

The new component enters edit-in-context directly.

### Save externally

Support:

```text
Save Virtual Component Externally
```

This must:
- create a normal external authoritative Duomec part/assembly file;
- replace the embedded definition reference with the external reference;
- preserve occurrence IDs, transform, mates and contextual references where valid.

For a virtual subassembly, allow:
- save only the subassembly externally while children remain embedded;
- save subassembly and selected/all embedded children externally.

Do not force virtual components to become external merely to edit them.

---

# 19D. Create Subassembly from selected components

Implement:

```cpp
class FormSubassemblyCommand;
```

The user can select parts/subassemblies at the same hierarchy level and execute:

```text
Create / Form Subassembly
```

The new subassembly may be:
- Virtual inside the current parent;
- External file.

### Preserve world-space position exactly

Default strategy:

```text
new subassembly local transform = Identity in current parent
selected occurrence transforms are reparented with equivalent local transforms
```

No visible component may jump.

Optionally allow the user to define the new subassembly frame from:
- parent assembly frame;
- active selected component origin;
- selected coordinate system/frame.

### Relation migration

For relations where **all endpoints are inside the selected set**:
- move ownership into the new subassembly.

For relations connecting a selected child to an unselected external occurrence:
- keep the relation at the parent level;
- update its occurrence path/reference through the new subassembly occurrence.

Relations must not be rebuilt from geometry unnecessarily.

### Performance

This operation is primarily:
- hierarchy rewrite;
- occurrence-path rewrite;
- relation-ownership rewrite.

It must not:
- recompute part B-Reps;
- retessellate parts;
- duplicate definition assets.

---

# 19E. Make Independent

Implement:

```cpp
class MakeIndependentCommand;
```

Use case:
multiple occurrences currently reference the same part/subassembly definition; the user wants selected occurrence(s) to diverge from future edits to the original.

### Semantics

For selected occurrences:

```text
old DefinitionId
       ↓
new independent DefinitionId
```

Preserve:
- occurrence IDs;
- transforms;
- mates;
- visibility;
- occurrence metadata.

Other unselected occurrences continue referencing the original definition.

### Copy-on-write optimization

Logical independence does not require immediate physical cache duplication.

Initially:

```text
Old Definition ContentHash == New Definition ContentHash
```

therefore:
- exact geometry cache may be shared;
- display cache may be shared;
- edge/BVH cache may be shared.

On the first edit to the independent definition:
- create new revision/hash;
- only affected assets diverge.

This is the preferred Duomec behavior because it preserves SOLIDWORKS-like user semantics while remaining compatible with the large-assembly memory strategy.

### External vs virtual

Allow:
- independent external component;
- independent virtual component.

If an external new file is requested, materialize it atomically.

---

# 19F. Component lifecycle commands must preserve relations

For:

```text
Replace Component
Make Independent
Create Subassembly
Virtual -> External
External -> Virtual (if implemented)
```

the general rule is:

> Occurrence identity and relation semantics should survive whenever the user's logical instance survives.

Do not solve lifecycle operations by deleting the occurrence and reinserting a visually identical new one unless unavoidable.

This preserves:
- mate IDs;
- downstream assembly references;
- future motion semantics;
- future FEM analysis-reference provenance.

---

# 20. Direct manipulation — free components

This behavior is mandatory.

## Left mouse button

When one or more selected occurrences are unconstrained/free:

```text
LMB drag = translate selected occurrence/group
```

Default translation behavior:

**camera/screen plane translation at the selected pivot depth**.

Reason:
- predictable direct manipulation;
- no need to infer an arbitrary world axis;
- consistent in perspective and orthographic views.

The optional transform triad remains available for axis-specific movement.

## Right mouse button

When one or more selected occurrences are free:

```text
RMB drag = rotate selected occurrence/group
```

Use an intuitive virtual-trackball/arcball style interaction.

Do not confuse RMB-drag with context-menu click:
- short click/release without significant movement = context menu;
- drag exceeding threshold = rotate.

This must be configurable if platform conventions require it.

---

# 21. Rotation pivot policy

Do not load full part authoring or exact B-Rep merely to obtain a pivot.

Milestone 5.1 part metadata must carry the component's **local part origin frame** as lightweight metadata.

Pivot priority:

```text
1. Explicit user pivot / selected reference
2. Active component local origin
3. Multi-selection group bounding-box center
4. Single component bounding-box center fallback
```

Do not default to center of mass because:
- it may require mass-property calculation/material;
- it can move after design changes;
- it is less predictable for assembly placement.

Center of mass can be offered as an optional pivot later.

Therefore the optimization concern does **not** justify losing the part origin: origin/frame metadata is tiny and should always be available.

---

# 22. Multi-selection free drag

If multiple free components are selected:

```text
LMB drag
```

moves them as a temporary rigid selection group, preserving mutual transforms.

```text
RMB drag
```

rotates them as a temporary group about:
- active occurrence origin if explicitly chosen;
- otherwise group bounding-box center.

This does not create a persistent rigid group unless the user requests one.

One mouse gesture creates one undo transaction.

---

# 23. Partially constrained drag

For constrained components, direct manipulation must respect remaining DOFs.

Do not disable dragging merely because a component has mates.

Flow:

```text
mouse target
 -> desired translational/rotational target
 -> identify affected solve island
 -> warm-start current valid transforms
 -> solve only island
 -> show feasible motion
```

If only one rotational DOF remains:
- drag moves/rotates only around that DOF.

If one translational DOF remains:
- pointer motion projects onto that DOF.

If no DOF remains:
- component stays fixed;
- show a subtle fully-constrained cue.

This behavior becomes the foundation for future mechanism manipulation.

---

# 24. Interactive drag solver

Interactive solve and final solve may use different computational budgets.

During pointer movement:

```text
InteractiveSolve
```

Requirements:
- local solve island only;
- warm start from previous frame;
- bounded iteration/time budget;
- cancellation/replacement by newer pointer event;
- never block UI waiting for obsolete drag solution.

Display the latest valid feasible pose.

On mouse release:

```text
FinalSolve
```

must:
- use full configured convergence;
- validate relations;
- either commit one transaction;
- or revert to last valid pose and report failure.

Never commit an unconverged final pose silently.

---

# 25. Solve-island graph

Create a graph:

```cpp
class AssemblyRelationGraph;
class SolveIsland;
class SolveIslandManager;
```

Nodes:
- occurrences or rigid groups.

Edges:
- active relations.

A new or edited relation invalidates only its connected solve island.

Independent assembly branches must not be solved.

When relations are added/removed:
- merge/split islands incrementally.

Grounded occurrence acts as ground node.

---

# 26. Ground / Fix component

Support explicit occurrence grounding separate from Lock mate.

```text
GroundOccurrence
```

means:
- occurrence transform fixed relative to current assembly frame;
- no second component required.

This is useful for:
- root/base part;
- mechanism ground;
- multibody world body.

Future MBD:
- map directly to fixed/grounded rigid body.

---

# 27. Component mobility and flexible/rigid behavior

The UI must give users the familiar professional controls:

```text
Fix
Float / Mobile
Make Flexible / Make Rigid   // when the component type supports it
```

However, **do not model Fixed/Floating/Flexible as one simplistic enum internally**.

They describe different aspects.

## 27.1 Placement mobility

```cpp
enum class PlacementMobility {
    Fixed,
    Floating
};
```

### Fixed

The occurrence pose is fixed relative to its parent assembly frame.

- zero occurrence-level placement DOF;
- no mate is required to hold that pose;
- changing `Fixed` to `Floating` restores normal mate/DOF behavior;
- Fixed is not stored as six fake mate equations.

### Floating / Mobile

Occurrence pose is governed by:
- mates;
- grounding of connected groups;
- remaining DOF.

An unconstrained floating part has six relative rigid-body DOF.

## 27.2 Subassembly solve mode

```cpp
enum class SubassemblySolveMode {
    Rigid,
    Flexible
};
```

### Rigid subassembly

Default.

The parent assembly treats the subassembly as one condensed rigid node.

Its internal mates are not expanded into the parent solve island.

### Flexible subassembly

Internal occurrence DOF and internal relations are exposed into the parent solve context only as required.

This allows mechanisms inside the subassembly to move while respecting:
- internal mates;
- parent-level mates.

For performance:
- do not permanently flatten the subassembly;
- dynamically expand the required internal solve graph;
- cache the condensed rigid representation for when Flexible is turned off.

Different occurrences of the same subassembly definition may be Rigid or Flexible independently.

## 27.3 Flexible parts

A part may expose a `FlexiblePart` mode only if it was authored with valid flexible/contextual references that can be remapped.

Create a separate mechanism such as:

```cpp
struct FlexiblePartContext {
    vector<FlexibleReferenceBinding> bindings;
};
```

Do not treat an ordinary rigid part as physically deformable merely because the user selects “Flexible”.

This Milestone does not implement structural deformation.

## 27.4 Combinations

Examples:

```text
Fixed + Rigid subassembly
    -> whole subassembly frozen.

Floating + Rigid subassembly
    -> subassembly moves as one rigid body.

Floating + Flexible subassembly
    -> parent solver may propagate motion into internal mechanism.

Fixed + Flexible subassembly
    -> subassembly root pose fixed, internal mechanism may still move.
```

This two-axis model is required to preserve correct semantics and performance.

---

# 28. Solver input must remain geometry-light

A solve island contains compact data:

```cpp
struct SolverBodyState {
    OccurrenceId id;
    Transform transform;
    bool grounded;
};

struct SolverConstraintRecord {
    AssemblyRelationId relationId;
    SolverConstraintType type;
    CompactFrame frameA;
    CompactFrame frameB;
    ConstraintParameters parameters;
};
```

No `TopoDS_Face`, `TopoDS_Edge`, AIS object, tessellation or Qt object inside the solver hot path.

---

# 29. Incremental solving and warm start

Never solve every edit from identity.

Use:
- current occurrence transforms;
- last valid island solution;
- previous drag frame solution.

Adding one relation:

```text
current valid assembly state
+ one new constraint
 -> nearest feasible solution
```

Prefer minimal displacement of unconstrained components where multiple valid solutions exist.

This is important for intuitive mate creation.

---

# 30. Mate placement preview

When creating a relation:
- compute a transient preview;
- do not commit immediately;
- show alignment direction;
- show component movement.

If multiple valid branches exist:
- choose nearest branch to current pose;
- allow `Flip`.

This reduces unexpected jumps.

---

# 31. Anti-flip and branch persistence

Angle and orientation relations often have multiple mathematically valid solutions.

Persist branch/orientation state.

Create:

```cpp
struct RelationBranchState {
    AlignmentMode alignment;
    int solutionBranch;
    optional<ReferenceDirection> stabilizer;
};
```

On reopen/recompute, maintain the intended branch where still valid.

Do not allow a mate to flip by 180° merely because numerical initialization changed.

---

# 32. Diagnostics and relation states

Required relation states:

```text
Solved
Underconstrained
Redundant
Conflicting
DanglingReference
Suppressed
InactiveGrounded
SolverFailed
NeedsReview
```

Every relation has:
- machine-readable status;
- user-readable explanation;
- involved occurrences;
- involved references;
- residual/error;
- DOF effect where calculable.

---

# 33. Mate error visualization

Do not limit errors to the tree.

When a relation fails:
- highlight relation references;
- show glyph/callout;
- offer actions near geometry.

Example:

```text
Concentric Mate 17
Reference B no longer resolves.

[Replace reference]
[Suppress]
[Delete]
```

For conflict:

```text
Mate 21 conflicts with:
Mate 8
Mate 14
```

Highlight the smallest known conflict set where solver diagnostics allow it.

---

# 34. Replace / repair mate references

Implement a professional repair flow.

When topology changes:
1. run Duomec persistent topology resolution;
2. if exact match succeeds, update compact descriptor automatically;
3. if ambiguous, mark relation `NeedsReview`;
4. offer candidate replacement references;
5. preview before commit.

Create:

```cpp
class RelationReferenceRepairService;
```

Never bind a dangling mate to an arbitrary “closest” face silently.

---

# 35. Overconstraint / redundancy management

This matters both for CAD usability and future multibody dynamics.

The solver/diagnostic layer must identify:
- relation causing conflict;
- relation adding no new independent constraint;
- closed-loop redundancy.

Offer:
- suppress candidate;
- convert compatible mate set to high-level joint;
- retain redundant relation for design intent but flag for motion export;
- later convert to bushing/compliance in analysis if desired.

Do not automatically delete user relations.

---

# 36. Kinematic joint composer

Create:

```cpp
class KinematicJointRecognizer;
class KinematicJointComposer;
```

Purpose:
identify semantic joint patterns from ordinary mates.

Examples:

```text
Concentric + Coincident
 -> Revolute candidate
```

```text
Parallel-axis constraints + translation-only freedom
 -> Prismatic candidate
```

```text
Coincident point/center constraints allowing 3 rotations
 -> Spherical candidate
```

This is primarily metadata/analysis preparation.

Do not rewrite the user's mate tree automatically without confirmation.

---

# 37. Motion semantic descriptor

Every relation carries:

```cpp
struct MotionSemanticDescriptor {
    MotionRelationKind kind;

    KinematicFrame frameA;
    KinematicFrame frameB;

    DofMask allowedRelativeDofs;

    MotionParameters parameters;

    bool directJointMapping;
};
```

Examples:

Hinge:
```text
kind = Revolute
directJointMapping = true
```

Screw:
```text
kind = Screw
lead = ...
directJointMapping = true
```

Generic Coincident:
```text
kind = GenericConstraint
directJointMapping = false
```

This metadata is authoritative enough for a later motion compiler.

---

# 38. Future Project Chrono bridge

Create interface only:

```cpp
class IMultibodyRelationExporter;
class IKinematicModelCompiler;
```

Optional prototype:

```cpp
class ChronoRelationMappingPrototype;
```

Do **not** run full dynamics in M5.2.

Document planned mappings, for example:

```text
Ground           -> fixed body
Lock             -> fixed link
Hinge            -> revolute
Slot/slider case -> prismatic/path/generic depending semantics
Universal        -> universal
Screw            -> screw
Gear             -> gear relation
RackPinion       -> rack-pinion relation
Belt             -> pulley/belt relation
Path             -> trajectory relation
```

Project Chrono provides rigid-body joints/constraints including fixed, revolute, prismatic, spherical, universal, screw, gear/rack-pinion/pulley-style relationships and generic mate constraints. The later implementation should use the highest-level stable Chrono primitive that preserves intended DOF.

---

# 39. Reaction/load frame preparation

SOLIDWORKS Motion later reports forces/reactions through joints/mates.

Prepare now by storing stable relation frames.

Add optional:

```cpp
struct AnalysisRelationMetadata {
    KinematicFrame reactionFrame;
    vector<TopologyReference> optionalLoadTransferReferences;
};
```

This does not calculate loads in M5.2.

It ensures future dynamics/FEM workflows know:
- where joint reactions are defined;
- which local coordinate system to use;
- optional physical load-bearing faces selected by the user.

---

# 39A. On-demand Multibody compilation — no runtime penalty in normal assembly work

Assembly relations define kinematic intent, but **Project Chrono objects must not exist in the normal assembly runtime**.

Normal assembly session stores only compact Duomec data:

```text
Relation type
Occurrence IDs
Local frames
Geometry descriptors
Parameters
DOF semantics
MotionSemanticDescriptor
```

Only when the user enters the future Motion/Multibody environment:

```text
Assembly model
    ↓
IKinematicModelCompiler
    ↓
compile selected/current mechanism scope
    ↓
Project Chrono model
```

This prevents multibody preparation from adding routine solve/render/load overhead to ordinary CAD assembly work.

The compiler may cache the compiled analysis model by:
- assembly relation graph hash;
- participating part revision hashes;
- motion-study settings.

A changed unrelated component must not invalidate the mechanism model if it is outside the analysis scope.

---

# 39B. On-demand FEM relation extraction

Assembly mates are useful **references and semantic hints** for structural analysis, but CAD positioning constraints are **not automatically physical contact conditions**.

For example:

- Coincident planar faces may represent bonded, sliding, contacting, or simply aligned components.
- Concentric mate does not prove bearing contact.
- Distance mate does not imply a physical connector.
- Gear mate defines kinematic ratio, not tooth-contact stiffness.

Therefore do not generate persistent FEM contact objects during normal assembly work.

Create an interface for the future CAE environment:

```cpp
class IAssemblyToFemCompiler;
class FemRelationHintExtractor;
```

When the user enters FEM:

```text
Assembly occurrences + relations
       ↓
resolve only analysis-scope relation references
       ↓
generate FEM setup candidates
       ↓
user reviews/confirms
       ↓
CAE model
```

Possible candidate mappings:

```text
Coincident faces
    -> candidate contact pair / bonded pair / interface pair

Concentric / Hinge
    -> candidate cylindrical connector / joint / bearing-like support

Lock / Fixed relation
    -> candidate rigid/bonded connector semantics

Remote frame / origin relation
    -> candidate remote point / connector frame

Symmetric relation
    -> possible symmetry-reference hint

Mechanical relations
    -> connector/kinematic hints, not automatically structural contact
```

### Optional analysis hint metadata

Relations may carry lightweight optional metadata:

```cpp
enum class AnalysisHintKind {
    None,
    AutoCandidate,
    ContactCandidate,
    BondedCandidate,
    JointCandidate,
    ConnectorCandidate,
    BoundaryConditionCandidate
};

struct AnalysisRelationHint {
    AnalysisHintKind kind;
    optional<KinematicFrame> frame;
};
```

Default must be:

```text
None or AutoCandidate
```

depending on the relation type.

This metadata is tiny and does not instantiate FEM entities.

### Exact geometry

FEM relation extraction may require the exact referenced faces.

Resolve/load them **only when compiling the FEM analysis scope**, using the M5.1 residency system.

Do not keep exact B-Rep of every mated component resident merely because the relations could later be used for FEM.

### CAE cache separation

Any generated:
- contact pair;
- connector;
- mesh relation;
- Code_Aster setup;

belongs to a separate CAE document/cache layer.

It must not pollute the assembly runtime invalidation path.

---

# 39C. Shared provenance for CAD, Motion and FEM

Every relation must maintain enough provenance to answer:

```text
Which occurrences?
Which persistent references?
Which local frames?
Which geometry descriptors?
Which revision produced the current descriptor?
```

The same relation can then be used independently by:

```text
Assembly constraint solver
Motion compiler
FEM setup compiler
```

without the three systems owning or mutating each other's runtime objects.

Architecture:

```text
                Duomec AssemblyRelation
                       |
          +------------+-------------+
          |                          |
Design-time solver             Analysis compilers
          |                    /             \
 occurrence pose         Motion compiler    FEM compiler
                              |                 |
                           Chrono           Code_Aster/etc.
```

This separation is non-negotiable for performance and maintainability.

---

# 40. Relationship browser

Assembly tree must expose a dedicated relation section.

Support:
- mates by component;
- mates by status;
- user folders;
- mechanical relations;
- solve island / mechanism debug view.

Selecting a relation:
- highlights entities;
- displays compact relation callout.

Do not expand/load exact geometry for every mate merely to list relations.

---

# 41. View Mates workflow

For selected occurrence(s):

```text
View Relations
```

must show only related relations.

Optionally:
- fade unrelated components;
- isolate connected solve island;
- show relation glyphs.

This is useful for diagnosing professional assemblies.

---

# 42. Mate candidate inference must be deterministic

The `MateCandidateEngine` may rank suggestions using heuristics, but it must be explainable.

Candidate score inputs may include:
- geometry compatibility;
- current orientation;
- current distance;
- common mechanical pattern;
- part Mate References;
- whether the mate would immediately conflict;
- user recent-choice preference only as a secondary factor.

Do not use an opaque ML model for baseline mate inference.

---

# 43. Default mate examples

Rules should roughly match professional expectations:

```text
parallel planar faces -> Coincident primary
cylindrical/circular coaxial geometry -> Concentric primary
two vertices -> Coincident primary
axis + cylinder -> Concentric
```

If primary default is incompatible with current constraint state but a secondary valid relation is obvious, candidate engine may highlight the secondary option while explaining why.

Do not silently create a different relation than the button selected by the user.

---

# 44. Performance requirements

M5.2 inherits M5.1 instrumentation.

Measure:
- quick-mate candidate generation;
- preview solve;
- final mate solve;
- drag solve frame time;
- solve-island size;
- relation graph update;
- reference descriptor extraction;
- detailed geometry load triggered by relation creation.

Report median/p95/p99.

Key architecture assertions:

- adding a mate must not solve unrelated islands;
- dragging one mechanism must not solve unrelated islands;
- solver hot path contains no B-Rep traversal for already-resolved relations;
- face/edge geometry is not globally loaded for relation display;
- adding one mate does not retessellate parts;
- moving under constraints does not modify exact part geometry.

---

# 45. Interactive budgets

Do not invent marketing numbers before benchmarking reference hardware.

Classify:

```text
Quick candidate calculation = interactive
Quick mate preview          = interactive
Free component drag         = frame-critical
Constrained drag            = frame-critical / bounded iterative
Final mate solve            = short interactive
Large closed-loop solve     = may exceed one frame but must stay local and show progress if needed
```

For constrained drag:
- prioritize continuity/smoothness;
- solve only current island;
- discard stale pointer solutions;
- final exact solve at release.

---

# 46. Test corpus

Create deterministic assemblies:

```text
m5_2_free_two_parts
m5_2_standard_mates
m5_2_concentric_lock_rotation
m5_2_angle_antiflip
m5_2_limit_distance
m5_2_limit_angle
m5_2_width_modes
m5_2_slot_modes
m5_2_path
m5_2_profile_center
m5_2_linear_coupler
m5_2_gear
m5_2_rack_pinion
m5_2_screw
m5_2_hinge
m5_2_universal
m5_2_cam
m5_2_belt
m5_2_quick_insert
m5_2_mate_reference_insertion
m5_2_closed_four_bar
m5_2_redundant_loop
m5_2_conflicting_mates
m5_2_dangling_reference
m5_2_flexible_subassembly
m5_2_large_many_small_islands
m5_2_large_single_island
```

---

# 47. Validation per relation

For each mate type test:

- final relative transform;
- residual;
- expected remaining DOF;
- suppression/unsuppression;
- save/reload;
- undo/redo;
- reference repair;
- current branch/alignment;
- future motion semantic descriptor.

Mechanical relations also test:
- expected motion ratio;
- sign/direction;
- phase/offset.

---

# 48. SOLIDWORKS compatibility checklist

The target functional vocabulary for M5.2 includes at minimum:

## Standard
- Angle
- Coincident
- Concentric
- Distance
- Lock
- Parallel
- Perpendicular
- Tangent

## Advanced
- Limit Angle
- Limit Distance
- Linear/Linear Coupler
- Path
- Profile Center
- Symmetric
- Width

## Mechanical
- Cam
- Gear
- Hinge
- Rack and Pinion
- Screw
- Slot
- Universal Joint

## Assembly relationship/workflow support
- Belt/Chain relation
- In Place
- Lock To Sketch / generated-path attachment
- coordinate/frame relationship
- Smart/Quick mate inference
- Mate References
- magnetic/connection-point architecture

Do not declare M5.2 feature-complete while one of the major standard/advanced/mechanical mate families is silently absent.

---

# 49. Deliberate UX improvements beyond SOLIDWORKS-style workflow

Preserve familiar behavior, but improve it.

## Improvement 1 — Quick Mate always available

No settings switch should be required for the core Ctrl-select contextual mate toolbar.

Allow user preference to disable it, but default ON.

## Improvement 2 — Show remaining DOF immediately

After a mate preview/commit display a subtle temporary status:

```text
Component: 1 rotational DOF remaining
```

Optionally show small axis glyph.

## Improvement 3 — Mate impact preview

Before committing:
- show which component(s) will move;
- indicate if a solve island is large;
- warn if relation is redundant/conflicting.

## Improvement 4 — One-click mechanical composite

Offer Hinge/Insert/Slot directly when geometry strongly supports the semantic relation.

## Improvement 5 — Motion-ready indicator

Mechanical relation inspector can show:

```text
Motion mapping: Revolute joint ✓
```

or:

```text
Motion mapping: Generic constraints — review recommended
```

This helps users build mechanisms that will transfer cleanly to simulation later.

---

# 50. Scope intentionally deferred

Do not implement full multibody dynamics now:

- time integration;
- forces;
- torques;
- motors;
- springs/dampers;
- friction;
- contacts;
- gravity solution;
- reaction plots;
- motion timeline;
- event-based motion;
- flexible bodies;
- FEA coupling;
- motion-derived FEM load cases.

These belong to the later Duomec Motion milestone using Project Chrono.

Also defer advanced collision-stop-during-drag unless baseline relation solving is already robust.

---

# 51. Required sub-milestones — revised

## M5.2A — Relation semantics, absolute references, component state and DOF framework

Implement:
- full relation taxonomy;
- endpoints;
- compact geometry descriptors;
- local kinematic frames;
- absolute origin/Front/Top/Right/X/Y/Z reference metadata;
- DOF model;
- motion semantic descriptor;
- analysis hint metadata;
- `PlacementMobility` Fixed/Floating;
- `SubassemblySolveMode` Rigid/Flexible;
- persistence.

No full solver UI yet.

STOP.

## M5.2B — Solver adapter + standard mates

Evaluate/integrate the OndselSolver adapter.

Implement all standard mates:
- coincident;
- concentric;
- distance;
- angle;
- parallel;
- perpendicular;
- tangent;
- lock;
- ground occurrence;
- origin/plane/axis/frame mates.

Add:
- branch/alignment;
- lock rotation;
- incremental solve;
- solve islands.

STOP.

## M5.2C — Direct manipulation + Quick Mates

Implement:
- LMB translate;
- RMB rotate;
- multi-selection group manipulation;
- constrained drag;
- cursor-adjacent Quick Mate toolbar;
- candidate engine;
- inline values;
- Flip;
- origin/plane/frame Quick Mates;
- Insert composite;
- Hinge quick relation.

STOP.

## M5.2D — Component lifecycle and hierarchy operations

Implement:
- no-placement insertion = origin/primary-planes aligned + Fixed, without fake mates;
- explicit cursor placement = preserve pose + Floating by default;
- Replace Component;
- reference/mate migration;
- Virtual Part;
- Virtual Subassembly;
- Save Virtual Component Externally;
- Create/Form Subassembly;
- relation migration during reparenting;
- Make Independent with copy-on-write cache sharing.

STOP.

## M5.2E — Advanced mates

Implement:
- limit distance;
- limit angle;
- linear coupler;
- path;
- profile center;
- symmetric;
- width modes.

STOP.

## M5.2F — Mechanical mates

Implement:
- cam;
- gear;
- hinge;
- rack and pinion;
- screw;
- slot;
- universal joint;
- belt/chain relation.

Validate kinematic ratios and remaining DOF.

STOP.

## M5.2G — Smart insertion / Mate References / connection points

Implement:
- primary/secondary/tertiary Mate References;
- auto snap during insertion;
- preferred mate semantics;
- optional magnetic/connection-point foundation.

STOP.

## M5.2H — Diagnostics / conflict / redundancy / repair

Implement:
- relation status;
- conflict localization;
- redundant mate diagnostics;
- reference repair;
- replacement-component mate reattachment UX;
- View Relations;
- relation browser.

STOP.

## M5.2I — Motion and FEM semantic compiler interfaces

Implement:
- kinematic joint recognition;
- motion descriptor validation;
- `IKinematicModelCompiler`;
- Project Chrono mapping report/prototype;
- `IAssemblyToFemCompiler`;
- FEM relation-hint extraction;
- no dynamics execution;
- no FEM solve/mesh execution.

STOP.

## M5.2J — Large assembly performance hardening

Benchmark:
- many independent islands;
- large mechanism island;
- repetitive Quick Mate use;
- constrained drag;
- flexible subassembly;
- Replace Component;
- Create Subassembly;
- Make Independent;
- virtual component editing;
- origin/plane mates using metadata-only references.

Integrate with M5.1 scheduler and profiler.


# 52. What Codex must do first

Before implementation:

1. Build M0–M5.1 repository.
2. Run every existing test and performance gate.
3. Read M5.1 occurrence/revision/invalidation ADRs.
4. Verify the 5.1 runtime exposes:
   - occurrence transforms;
   - stable occurrence IDs;
   - local part origin metadata;
   - Front/Top/Right primary-plane metadata;
   - X/Y/Z axis metadata;
   - local exact-selection references;
   - solve-island hooks;
   - transform invalidation without geometry invalidation;
   - definition/revision copy-on-write capability;
   - embedded/virtual asset support or a clear extension point.
5. If primary reference geometry is not currently available at `Metadata` residency, update M5.1 manifest/schema first. Do **not** work around this by loading B-Rep.
6. Evaluate `FreeCAD/OndselSolver`:
   - current license;
   - API;
   - build integration;
   - basic 3D constraint support;
   - incremental solving behavior.
7. Write:
   - `M5_2_SOLVER_EVALUATION.md`
   - `M5_2_RELATION_TAXONOMY.md`
   - `M5_2_COMPONENT_STATE_MODEL.md`
   - `M5_2_COMPONENT_LIFECYCLE.md`
   - `M5_2_MOTION_MAPPING.md`
   - `M5_2_FEM_RELATION_HINTS.md`
   - `M5_2_QUICK_MATE_UX.md`
8. Create ADRs:
   - relation semantic ownership;
   - endpoint/frame representation;
   - absolute reference geometry;
   - Fixed/Floating vs Rigid/Flexible state separation;
   - DOF convention;
   - solver adapter;
   - solve-island strategy;
   - constrained drag;
   - Quick Mate candidate engine;
   - Replace Component reference migration;
   - virtual component ownership/persistence;
   - Create Subassembly relation migration;
   - Make Independent copy-on-write behavior;
   - motion semantic bridge;
   - on-demand FEM extraction boundary.
9. Implement **M5.2A only**.
10. Build/run tests.
11. Stop for review.


# 53. Definition of Done — revised

Milestone 5.2 is complete only when:

## Mate coverage
- all required standard mate families work;
- all required advanced mate families work;
- all required mechanical mate families work;
- Belt/Chain relation architecture works;
- Mate References work;
- component and assembly origins/primary planes/axes can participate in relations without loading full B-Rep.

## Component state
- user can Fix/Float occurrences;
- rigid/flexible subassembly behavior is explicit and independent from Fix/Float;
- eligible flexible parts use controlled flexible-reference bindings;
- a fixed flexible subassembly can keep its root fixed while exposing internal mechanism DOF;
- state changes do not create fake mate records.

## Default insertion
- one or multiple components inserted without cursor placement are aligned by origin/Front/Top/Right frame to the current parent assembly and are Fixed;
- no artificial mates are created;
- explicit interactive placement preserves the chosen pose and defaults Floating unless configured otherwise.

## Component lifecycle
- Replace Component supports selected/all requested instances;
- replacement preserves occurrence identity/pose where possible;
- mates are reattached using deterministic priority and unresolved references are explicit;
- Virtual Part and Virtual Subassembly can be authored in context;
- virtual components can be saved externally without losing occurrence relations;
- selected components can be formed into a subassembly without visible movement or B-Rep rebuild;
- internal/cross-boundary relations migrate correctly;
- Make Independent creates new logical definition identity while preserving occurrence state;
- Make Independent reuses identical content-addressed caches until the new definition diverges.

## UX
- Ctrl-select compatible entities opens Quick Mate overlay near cursor;
- origins/planes/axes/coordinate frames work in Quick Mate;
- only valid relations are offered;
- default mate is sensible;
- Enter accepts;
- Tab flips where relevant;
- Distance/Angle/Limit can be entered inline;
- common mates do not require opening a full panel.

## Manipulation
- free LMB drag translates;
- free RMB drag rotates;
- rotation uses lightweight origin metadata and never loads authoring solely for pivot;
- multi-selection moves as a temporary rigid group;
- constrained components move within remaining DOF.

## Solver
- solve islands are local;
- warm start works;
- closed-loop tests exist;
- redundancy and conflict are distinguished;
- final drag release never commits an unconverged pose;
- solver hot path uses compact frames/descriptors rather than B-Rep traversal.

## Performance
- unrelated islands are not solved;
- no part retessellation occurs because of a mate solve;
- relation listing does not force exact geometry residency;
- origin/plane mates operate from lightweight metadata;
- Replace/Create Subassembly/Make Independent do not globally rebuild geometry;
- M5.1 performance guarantees remain intact.

## Robustness
- mate branches do not randomly flip;
- topological-reference changes trigger deterministic repair/review;
- dangling mates are explicit;
- replace-reference workflow works;
- Replace Component never silently discards failed mate references.

## Motion readiness
- every relation has DOF/kinematic semantics;
- direct high-level mappings exist for hinge, universal, gear, rack-pinion, screw and belt/pulley-like relations;
- generic relations retain enough information for later constraint export;
- stable reaction frames exist;
- Chrono objects are created only by an on-demand Motion compiler, not during normal CAD assembly work.

## FEM readiness
- relations retain persistent geometry/reference provenance useful to the future FEM environment;
- normal assembly operation does not instantiate FEM contact/connector objects;
- entering FEM can compile relation hints on demand;
- CAD mates are treated as candidate analysis semantics, not automatically assumed physical contacts;
- exact referenced geometry is loaded only for the selected CAE analysis scope;
- CAE caches remain separate from assembly runtime caches/invalidation.

## Compatibility
- all M0–M5.1 tests continue to pass.

At completion write:

`docs/milestones/M5_2_REPORT.md`

The report must include:
- mate coverage matrix;
- absolute-reference mate matrix;
- component-state matrix;
- component lifecycle test matrix;
- selection compatibility matrix;
- solver backend/version/license;
- closed-loop/redundancy results;
- Quick Mate UX results;
- direct-drag performance;
- large-island and many-island timings;
- Replace Component/reference-repair results;
- Virtual/Create Subassembly/Make Independent results;
- motion mapping table;
- FEM hint mapping table;
- known unsupported geometry combinations;
- numerical limitations;
- handoff contract for the future Duomec Motion / Project Chrono milestone;
- handoff contract for the future FEM / Code_Aster milestone.


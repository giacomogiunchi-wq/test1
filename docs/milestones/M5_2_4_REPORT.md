# Milestone 5.2 Step 5.2.4 report

## Architecture

Step 5.2.4 extends the solver-neutral `AssemblyRelation` vocabulary rather than introducing backend or UI types. Every advanced/mechanical record retains persistent endpoint references, compact descriptors, component-local frames, and semantic parameters. Solver adapters may compile these records transiently; persistence never replaces them with anonymous equations.

Diagnostics, reference repair, selected-component views, browser grouping, and joint recognition consume relation metadata only. They do not request B-Rep, tessellation, AIS, Qt, dynamics, or FEM objects.

## Relation coverage matrix

| Relation | Persisted semantic data | Structural remaining DOF |
|---|---|---:|
| Limit Distance | minimum, maximum, current, stable direction frame | 5 |
| Limit Angle | minimum, maximum, current, stable angular frame, branch | 5 |
| Linear Coupler | ratio, signed direction, offset, two frames | coupled scalar |
| Path | persistent path, free/distance/percent parameter, free/tangent orientation | 1 |
| Profile Center | circular/rectangular/regular polygon, alignment, orientation, offset, lock rotation | 1 |
| Symmetric | two references and symmetry plane | 5 |
| Width | width/tab references, center/free/distance/percent mode and value | 5 |
| Hinge | two frames, Revolute motion semantic | 1 rotational |
| Gear | two axes, ratio, direction, phase | coupled rotation |
| Rack and Pinion | rack/pinion axes, pitch radius or travel/revolution, direction, offset | coupled scalar |
| Screw | common axis, lead, handedness, phase, offset | 1 coupled helical |
| Slot | free/center/distance/percent mode and orientation behavior | 1 |
| Universal Joint | input/output frames, center, phase | 2 rotational |
| Cam | profile/path/surface reference, follower, side, follower geometry | contact candidate |
| Belt/Chain | axes, effective radii, wrap, ratio/phase, belt-length metadata | coupled rotation |

## Diagnostics and repair

The domain distinguishes Solved, Underconstrained, Redundant, Conflicting, DanglingReference, Suppressed, SolverFailed, and NeedsReview. Structural duplicate/conflict analysis reports the smallest pair it can prove. Reference repair attempts exact persistent resolution first, then compatible semantic/signature candidates. Ambiguous fallback is marked NeedsReview and requires an explicit preview selection; arbitrary nearest-geometry attachment is forbidden.

Selected-component relation views, optional fade sets, solve-island isolation, and browser grouping by component/status/mechanical type/folder/island use only compact metadata. Recognition proposes Revolute, Prismatic, or Spherical candidates and never changes user mates without confirmation.

## Persistence and history

New relation enum values, modes, ratios, phases, limits, and descriptors round-trip through the existing versioned canonical relation snapshot. The existing assembly snapshot history provides undo/redo for these semantic records. A four-hinge closed loop remains one solve island.

## Performance baseline

On the dependency-free runner, structural diagnostics over 10,000 relations (including 500 deliberately conflicting pairs) took **85,832 µs**. This is a repeatable microbenchmark baseline, not a workstation guarantee. Browsing/filtering remains linear in relation metadata; no exact geometry or display assets are loaded.

## Unsupported geometry combinations

- Profile Center validation records supported profile categories, but production extraction/compatibility for arbitrary spline or non-regular profiles is not implemented.
- Path relations preserve topology identity and parameters, but production curve projection, discontinuity handling, and closed-path branch tracking await a nonlinear geometry backend.
- Width expects authored width/tab references; automatic inference from arbitrary topology is not implemented.
- Cam persists profile/surface and follower semantics but does not calculate physical contact.
- Belt/Chain does not model tooth engagement, chain links, compliance, or collision.

## Solver and diagnostic limitations

- The current dependency-free native backend is a structural DOF/consistency classifier, not a production nonlinear kinematic solver. It does not numerically converge these advanced equations or simulate a four-bar trajectory.
- Conflict localization is pair-minimal only for structurally comparable endpoint/parameter records; globally minimal inconsistent subsets require capabilities from a future production solver adapter.
- Redundancy recognition is semantic/structural and does not yet prove algebraic dependence across heterogeneous relation combinations.
- Reference repair uses caller-provided exact resolution and compatible compact candidates; topology-signature generation belongs to the authoring/topology adapter.
- Relation glyphs, fade rendering, and browser widgets are domain view data only; viewport/Qt integration is deferred.

## Scope boundary

No multibody dynamics, physical contact dynamics, or FEM solving/conversion was added. Work stops at Step 5.2.4 pending review.

#include "duomec/assembly/constraint_runtime.hpp"

#include <cassert>

namespace {
duomec::assembly::RelationEndpoint
endpoint(duomec::cad::OccurrenceId occurrence,
         duomec::assembly::GeometryKind kind) {
  duomec::assembly::RelationEndpoint result;
  result.occurrenceId = std::move(occurrence);
  result.geometry.kind = kind;
  return result;
}
duomec::assembly::AssemblyRelation relation(duomec::assembly::RelationType type,
                                            duomec::cad::OccurrenceId first,
                                            duomec::cad::OccurrenceId second) {
  duomec::assembly::AssemblyRelation result;
  result.type = type;
  result.endpoints = {
      endpoint(std::move(first), duomec::assembly::GeometryKind::Plane),
      endpoint(std::move(second), duomec::assembly::GeometryKind::Plane)};
  return result;
}
} // namespace

int main() {
  using namespace duomec::assembly;
  OccurrenceState first = OccurrenceState::interactive({});
  OccurrenceState second = OccurrenceState::interactive({});
  OccurrenceState third = OccurrenceState::interactive({});

  // Every standard semantic is representable without backend types.
  for (const auto type :
       {RelationType::Coincident, RelationType::Distance, RelationType::Angle,
        RelationType::Parallel, RelationType::Perpendicular,
        RelationType::Tangent, RelationType::Lock}) {
    const auto value = relation(type, first.id, second.id);
    assert(value.type == type && value.endpoints.size() == 2);
  }
  const auto grounded = groundOccurrence(first.id);
  assert(grounded.type == RelationType::Fixed &&
         grounded.endpoints.size() == 1);

  const auto absolute = LightweightReferenceMetadata::create();
  auto originEndpoint = endpoint(first.id, GeometryKind::Point);
  originEndpoint.topologyReferenceId =
      absolute.at(AbsoluteReferenceKind::Origin).referenceId;
  auto planeEndpoint = endpoint(second.id, GeometryKind::Plane);
  planeEndpoint.topologyReferenceId =
      absolute.at(AbsoluteReferenceKind::FrontPlane).referenceId;
  assert(originEndpoint.geometry.kind == GeometryKind::Point);
  assert(planeEndpoint.geometry.kind == GeometryKind::Plane);

  auto concentric = makeConcentric(endpoint(first.id, GeometryKind::Axis),
                                   endpoint(second.id, GeometryKind::Cylinder),
                                   {Alignment::AntiAligned, true});
  assert(std::get<bool>(concentric.parameters.at("lock_rotation")));
  assert(std::get<std::int64_t>(concentric.parameters.at("alignment")) == 1);
  auto angle = makeAngle(endpoint(first.id, GeometryKind::Plane),
                         endpoint(second.id, GeometryKind::Plane),
                         {{0, 1, 0}, Alignment::AntiAligned, -0.5});
  assert(std::get<double>(angle.parameters.at("reference_y")) == 1);
  assert(std::get<double>(angle.parameters.at("angle_rad")) == -0.5);
  const auto insert = makeInsert(endpoint(first.id, GeometryKind::Cylinder),
                                 endpoint(second.id, GeometryKind::Plane),
                                 {Alignment::Aligned, false});
  assert(insert.type == RelationType::Insert);
  assert(insert.motion.semantic == MotionSemantic::Revolute);

  // Islands merge locally and split when the bridge is removed.
  AssemblyRelationGraph graph;
  const auto firstSecond =
      relation(RelationType::Coincident, first.id, second.id);
  const auto secondThird =
      relation(RelationType::Parallel, second.id, third.id);
  assert(graph.add(firstSecond));
  assert(graph.add(secondThird));
  assert(graph.islands().size() == 1);
  assert(graph.islands().front().occurrences.size() == 3);
  assert(graph.remove(secondThird.id));
  assert(graph.islands().size() == 1);
  assert(graph.islands().front().occurrences.size() == 2);
  assert(!graph.islandFor(third.id));

  NativeAssemblyConstraintSolver solver;
  std::vector<OccurrenceState> occurrences{first, second};
  std::vector<AssemblyRelation> under{
      relation(RelationType::Distance, first.id, second.id)};
  under.front().parameters["distance_si"] = 1.0;
  const auto underResult = solver.solve(occurrences, under, {});
  assert(underResult.status == SolveStatus::UnderConstrained);
  auto duplicate = under;
  duplicate.push_back(under.front());
  duplicate.back().id = duomec::cad::AssemblyRelationId::generate();
  assert(solver.solve(occurrences, duplicate, {}).status ==
         SolveStatus::Redundant);
  auto conflict = duplicate;
  conflict.back().parameters["distance_si"] = 2.0;
  assert(solver.solve(occurrences, conflict, {}).status ==
         SolveStatus::Conflicting);

  // Quick Mate candidate filtering and inline values.
  MateCandidateEngine candidates;
  const GeometryDescriptor plane{GeometryKind::Plane};
  const GeometryDescriptor cylinder{GeometryKind::Cylinder};
  assert(candidates.candidates(plane, plane).size() == 4);
  assert(candidates.candidates(cylinder, cylinder).size() == 2);
  assert(candidates.candidates({GeometryKind::Sphere}, plane).empty());
  QuickMateController quick;
  quick.begin(endpoint(first.id, GeometryKind::Plane),
              endpoint(second.id, GeometryKind::Plane));
  assert(quick.overlay().visible);
  quick.flip();
  assert(quick.highlight(QuickMateCandidate::Distance));
  quick.setInlineValue(0.25);
  const auto quickRelation = quick.accept();
  assert(quickRelation && quickRelation.value().type == RelationType::Distance);
  assert(std::get<double>(quickRelation.value().parameters.at("distance_si")) ==
         0.25);
  assert(!quick.overlay().visible);

  // Transform-only free, grouped, and constrained manipulation.
  ComponentManipulator manipulator;
  manipulator.begin({first, second}, std::nullopt, Vector3{0, 0, 0});
  assert(manipulator.update(PointerButton::Left, {{1, 2, 0}, 0, true}) ==
         ManipulationOutcome::Preview);
  assert(manipulator.preview()[0].localTransform.matrix[12] == 1);
  assert(manipulator.preview()[1].localTransform.matrix[13] == 2);
  assert(manipulator.finish(true) == ManipulationOutcome::Committed);
  auto axial = first;
  axial.dofState.free = {true, false, false, false, false, false};
  manipulator.begin({axial});
  assert(manipulator.update(PointerButton::Left, {{2, 3, 4}, 0, true}) ==
         ManipulationOutcome::Preview);
  assert(manipulator.preview()[0].localTransform.matrix[12] == 2);
  assert(manipulator.preview()[0].localTransform.matrix[13] == 0);
  manipulator.begin({OccurrenceState::defaultOriginAligned()});
  assert(manipulator.update(PointerButton::Left, {{1, 0, 0}, 0, true}) ==
         ManipulationOutcome::FullyConstrained);
  manipulator.begin({first});
  assert(manipulator.update(PointerButton::Right, {{}, 0, false}) ==
         ManipulationOutcome::ContextMenu);
  manipulator.begin({first});
  assert(manipulator.update(PointerButton::Right, {{}, 0.5, true}) ==
         ManipulationOutcome::Preview);
  assert(manipulator.preview()[0].localTransform.matrix[0] != 1.0);
  assert(manipulator.finish(false) == ManipulationOutcome::Reverted);

  ConstrainedDragController drag(solver, graph);
  std::vector<OccurrenceState> all{first, second, third};
  drag.begin({first}, all, 4);
  assert(drag.update(PointerButton::Left, {{1, 0, 0}, 0, true}) ==
         ManipulationOutcome::Preview);
  assert(drag.lastSolve() && drag.lastSolve()->poses.size() == 2);
  assert(drag.finish() == ManipulationOutcome::Committed);
  drag.begin({first}, all, 5);
  drag.setCurrentRevision(6);
  assert(drag.update(PointerButton::Left, {{1, 0, 0}, 0, true}) ==
         ManipulationOutcome::Reverted);
  assert(drag.lastSolve()->status == SolveStatus::Stale);

  // Save/reload retains anti-flip/Insert data and history is one transaction.
  AssemblyRelationsSnapshot before;
  before.occurrences = occurrences;
  AssemblyRelationsSnapshot after = before;
  after.relations = {angle, concentric, insert};
  const auto loaded = deserialize(serialize(after));
  assert(loaded && loaded.value() == after);
  auto legacy = before;
  legacy.schemaVersion = 1;
  assert(deserialize(serialize(legacy)));
  AssemblyStateHistory history(before);
  history.commit(after);
  assert(history.undo() && history.current() == before);
  assert(history.redo() && history.current() == after);
}

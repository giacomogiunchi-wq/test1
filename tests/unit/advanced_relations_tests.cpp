#include "duomec/assembly/advanced_relations.hpp"

#include <cassert>

namespace {
duomec::assembly::RelationEndpoint
endpoint(duomec::cad::OccurrenceId occurrence,
         duomec::assembly::GeometryKind kind) {
  duomec::assembly::RelationEndpoint value;
  value.occurrenceId = std::move(occurrence);
  value.geometry.kind = kind;
  return value;
}
} // namespace

int main() {
  using namespace duomec::assembly;
  OccurrenceState a = OccurrenceState::interactive({});
  OccurrenceState b = OccurrenceState::interactive({});
  OccurrenceState c = OccurrenceState::interactive({});
  OccurrenceState d = OccurrenceState::interactive({});
  std::vector<AssemblyRelation> all;

  LimitDistanceDefinition distance{0.1, 0.5, 0.25, {}};
  auto limitDistance =
      makeLimitDistance(endpoint(a.id, GeometryKind::Plane),
                        endpoint(b.id, GeometryKind::Plane), distance);
  assert(limitDistance && std::get<double>(limitDistance.value().parameters.at(
                              "minimum_si")) == 0.1);
  assert(!makeLimitDistance(endpoint(a.id, GeometryKind::Plane),
                            endpoint(b.id, GeometryKind::Plane),
                            {1, 0, 0.5, {}}));
  all.push_back(limitDistance.value());

  LimitAngleDefinition angle{-1, 1, 0.2, {}, Alignment::AntiAligned};
  auto limitAngle = makeLimitAngle(endpoint(a.id, GeometryKind::Plane),
                                   endpoint(b.id, GeometryKind::Plane), angle);
  assert(limitAngle && std::get<std::int64_t>(
                           limitAngle.value().parameters.at("alignment")) == 1);
  all.push_back(limitAngle.value());

  auto coupler = makeLinearCoupler(
      endpoint(a.id, GeometryKind::Axis), endpoint(b.id, GeometryKind::Axis),
      {2.5, 0.02, DirectionSense::Negative, {}, {}});
  assert(coupler &&
         std::get<double>(coupler.value().parameters.at("ratio")) == 2.5);
  all.push_back(coupler.value());

  PathRelationDefinition pathDefinition{PathPositionMode::Percent,
                                        PathOrientationMode::FollowTangent, 0.4,
                                        12.75};
  auto path =
      makePathRelation(endpoint(a.id, GeometryKind::Point),
                       endpoint(b.id, GeometryKind::Path), pathDefinition);
  assert(path && std::get<double>(
                     path.value().parameters.at("path_parameter")) == 12.75);
  all.push_back(path.value());

  auto profile = makeProfileCenter(
      {endpoint(a.id, GeometryKind::Circle),
       endpoint(b.id, GeometryKind::Circle)},
      {ProfileShape::Circular, Alignment::Aligned, 0.1, 0.002, true});
  assert(profile &&
         std::get<bool>(profile.value().parameters.at("lock_rotation")));
  all.push_back(profile.value());

  auto symmetric = makeSymmetric(endpoint(a.id, GeometryKind::Point),
                                 endpoint(b.id, GeometryKind::Point),
                                 endpoint(c.id, GeometryKind::Plane));
  assert(symmetric.type == RelationType::Symmetric);
  all.push_back(symmetric);

  auto width = makeWidth({endpoint(a.id, GeometryKind::Plane),
                          endpoint(a.id, GeometryKind::Plane),
                          endpoint(b.id, GeometryKind::Plane),
                          endpoint(b.id, GeometryKind::Plane)},
                         {WidthMode::Percent, 0.35});
  assert(width &&
         std::get<double>(width.value().parameters.at("value")) == 0.35);
  all.push_back(width.value());

  auto hinge = makeHinge(endpoint(a.id, GeometryKind::Axis),
                         endpoint(b.id, GeometryKind::Plane));
  assert(hinge.motion.semantic == MotionSemantic::Revolute);
  assert(expectedRemainingDofs(hinge) == 1);
  all.push_back(hinge);

  auto gear = makeGear(endpoint(a.id, GeometryKind::Axis),
                       endpoint(b.id, GeometryKind::Axis),
                       {-3, DirectionSense::Negative, 0.25});
  assert(gear && std::get<double>(gear.value().parameters.at("ratio")) == -3);
  all.push_back(gear.value());

  auto rack = makeRackPinion(endpoint(a.id, GeometryKind::Line),
                             endpoint(b.id, GeometryKind::Axis),
                             {0.04, 0, DirectionSense::Positive, 0.01});
  assert(rack && rack.value().motion.semantic == MotionSemantic::RackPinion);
  all.push_back(rack.value());

  auto screw = makeScrew(endpoint(a.id, GeometryKind::Axis),
                         endpoint(b.id, GeometryKind::Axis),
                         {0.005, Handedness::LeftHanded, 0.1, 0.02});
  assert(screw &&
         std::get<double>(screw.value().parameters.at("lead_si")) == 0.005);
  all.push_back(screw.value());

  auto slot = makeSlot(
      endpoint(a.id, GeometryKind::Point), endpoint(b.id, GeometryKind::Slot),
      {SlotMode::Percent, PathOrientationMode::FollowTangent, 0.6});
  assert(slot && expectedRemainingDofs(slot.value()) == 1);
  all.push_back(slot.value());

  auto universal = makeUniversalJoint(
      endpoint(a.id, GeometryKind::CoordinateFrame),
      endpoint(b.id, GeometryKind::CoordinateFrame), {{1, 2, 3}, 0.3});
  assert(universal.motion.semantic == MotionSemantic::Universal);
  assert(expectedRemainingDofs(universal) == 2);
  all.push_back(universal);

  auto cam = makeCam(endpoint(a.id, GeometryKind::Curve),
                     endpoint(b.id, GeometryKind::Circle),
                     {ContactSide::Outside, FollowerGeometry::Roller});
  assert(cam.type == RelationType::Cam);
  all.push_back(cam);

  auto belt = makeBeltChain(
      {endpoint(a.id, GeometryKind::Axis), endpoint(b.id, GeometryKind::Axis)},
      {{0.1, 0.2}, WrapOrientation::Crossed, 0.4, 1.2});
  assert(belt &&
         std::get<double>(belt.value().parameters.at("belt_length_si")) == 1.2);
  all.push_back(belt.value());

  // Every advanced/mechanical semantic round-trips and participates in history.
  AssemblyRelationsSnapshot snapshot;
  snapshot.occurrences = {a, b, c, d};
  snapshot.relations = all;
  const auto loaded = deserialize(serialize(snapshot));
  assert(loaded && loaded.value() == snapshot);
  AssemblyStateHistory history;
  history.commit(snapshot);
  assert(history.undo());
  assert(history.redo() && history.current() == snapshot);

  // Redundancy remains distinct from a minimal two-relation conflict set.
  AssemblyRelation distanceA;
  distanceA.type = RelationType::Distance;
  distanceA.endpoints = {endpoint(a.id, GeometryKind::Plane),
                         endpoint(b.id, GeometryKind::Plane)};
  distanceA.parameters["distance_si"] = 1.0;
  auto distanceDuplicate = distanceA;
  distanceDuplicate.id = duomec::cad::AssemblyRelationId::generate();
  auto distanceConflict = distanceA;
  distanceConflict.id = duomec::cad::AssemblyRelationId::generate();
  distanceConflict.parameters["distance_si"] = 2.0;
  NativeAssemblyConstraintSolver solver;
  std::vector<OccurrenceState> pair{a, b};
  std::vector<AssemblyRelation> duplicateSet{distanceA, distanceDuplicate};
  auto duplicateSolve = solver.solve(pair, duplicateSet, {});
  assert(duplicateSolve.status == SolveStatus::Redundant);
  RelationDiagnosticService diagnostics;
  auto duplicateStates = diagnostics.classify(duplicateSet, duplicateSolve);
  assert(duplicateStates.at(distanceDuplicate.id) == RelationState::Redundant);
  std::vector<AssemblyRelation> conflictSet{distanceA, distanceConflict};
  auto conflicts = diagnostics.localize(conflictSet);
  assert(conflicts.size() == 1 && conflicts.front().relations.size() == 2);
  auto conflictStates =
      diagnostics.classify(conflictSet, solver.solve(pair, conflictSet, {}));
  assert(conflictStates.at(distanceA.id) == RelationState::Conflicting);
  distanceA.state = RelationState::Suppressed;
  auto suppressed = diagnostics.classify(
      std::span<const AssemblyRelation>(&distanceA, 1), {});
  assert(suppressed.at(distanceA.id) == RelationState::Suppressed);

  // Reference repair never chooses among ambiguous compatible candidates.
  DefinitionReferenceCatalog oldCatalog, newCatalog;
  auto oldReference = duomec::cad::TopologyReferenceId::generate();
  oldCatalog.descriptors[oldReference] = {GeometryKind::Cylinder};
  auto candidateOne = duomec::cad::TopologyReferenceId::generate();
  auto candidateTwo = duomec::cad::TopologyReferenceId::generate();
  newCatalog.descriptors[candidateOne] = {GeometryKind::Cylinder};
  newCatalog.descriptors[candidateTwo] = {GeometryKind::Cylinder};
  RelationEndpoint broken = endpoint(a.id, GeometryKind::Cylinder);
  broken.topologyReferenceId = oldReference;
  AssemblyRelation repairRelation;
  repairRelation.endpoints.push_back(broken);
  RelationReferenceRepairService repair;
  const auto repairPreview = repair.preview(
      broken, oldCatalog, newCatalog,
      [](const auto &) -> std::optional<duomec::cad::TopologyReferenceId> {
        return std::nullopt;
      });
  assert(repairPreview.resolution == RepairResolution::Ambiguous);
  assert(!repair.apply(repairRelation, broken.id, repairPreview, std::nullopt));
  assert(repairRelation.state == RelationState::NeedsReview);
  assert(repair.apply(repairRelation, broken.id, repairPreview, 1));

  // Four-bar relations stay one closed-loop island; no global graph is needed.
  AssemblyRelationGraph graph;
  std::array bars{a.id, b.id, c.id, d.id};
  std::vector<AssemblyRelation> fourBar;
  for (std::size_t index = 0; index < bars.size(); ++index) {
    auto joint = makeHinge(
        endpoint(bars[index], GeometryKind::Axis),
        endpoint(bars[(index + 1) % bars.size()], GeometryKind::Axis));
    fourBar.push_back(joint);
    assert(graph.add(joint));
  }
  assert(graph.islands().size() == 1 &&
         graph.islands().front().occurrences.size() == 4);

  // Views and browser grouping consume metadata only.
  ComponentOccurrence occurrenceA, occurrenceB, occurrenceC;
  occurrenceA.placement = a;
  occurrenceB.placement = b;
  occurrenceC.placement = c;
  std::vector<ComponentOccurrence> occurrences{occurrenceA, occurrenceB,
                                               occurrenceC};
  const std::array selected{a.id};
  RelationViewService viewService;
  const auto view =
      viewService.build(selected, occurrences, fourBar, graph, true, true);
  assert(!view.relations.empty() && view.fadedOccurrences.contains(c.id));
  RelationBrowserModel browser;
  assert(!browser.group(fourBar, RelationBrowserGrouping::Mechanical).empty());
  assert(!browser.group(fourBar, RelationBrowserGrouping::SolveIsland, &graph)
              .empty());

  // Recognition proposes but never rewrites until explicit confirmation.
  AssemblyRelation concentric;
  concentric.type = RelationType::Concentric;
  concentric.endpoints = {endpoint(a.id, GeometryKind::Axis),
                          endpoint(b.id, GeometryKind::Axis)};
  AssemblyRelation coincident;
  coincident.type = RelationType::Coincident;
  coincident.endpoints = {endpoint(a.id, GeometryKind::Plane),
                          endpoint(b.id, GeometryKind::Plane)};
  std::vector<AssemblyRelation> sources{concentric, coincident, path.value()};
  KinematicJointRecognizer recognizer;
  const auto candidates = recognizer.recognize(sources);
  assert(
      std::any_of(candidates.begin(), candidates.end(), [](const auto &value) {
        return value.kind == JointCandidateKind::Revolute;
      }));
  const auto revolute = *std::find_if(
      candidates.begin(), candidates.end(), [](const auto &value) {
        return value.kind == JointCandidateKind::Revolute;
      });
  KinematicJointComposer composer;
  assert(!composer.compose(revolute, sources, false));
  const auto composed = composer.compose(revolute, sources, true);
  assert(composed && composed.value().type == RelationType::Hinge);
}

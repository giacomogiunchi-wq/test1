#include "duomec/assembly/relations.hpp"

#include <cassert>
#include <set>

int main() {
  using namespace duomec::assembly;

  const auto refs = LightweightReferenceMetadata::create();
  std::set<std::string> ids;
  for (const auto &reference : refs.references)
    ids.insert(reference.referenceId.value());
  assert(ids.size() == 8);
  assert(refs.at(AbsoluteReferenceKind::Origin).kind ==
         AbsoluteReferenceKind::Origin);
  assert(refs.at(AbsoluteReferenceKind::FrontPlane).kind ==
         AbsoluteReferenceKind::FrontPlane);
  assert(refs.at(AbsoluteReferenceKind::TopPlane).kind ==
         AbsoluteReferenceKind::TopPlane);
  assert(refs.at(AbsoluteReferenceKind::RightPlane).kind ==
         AbsoluteReferenceKind::RightPlane);

  const auto free = DofState::floating();
  assert(free.independentDofCount() == 6 &&
         free.state == ConstraintState::UnderConstrained);
  for (int index = 0; index < 6; ++index)
    assert(free.isFree(static_cast<Dof>(index)));
  const auto fixed = DofState::fixed();
  assert(fixed.independentDofCount() == 0 &&
         fixed.state == ConstraintState::Fixed);

  for (const auto mobility :
       {PlacementMobility::Fixed, PlacementMobility::Floating}) {
    for (const auto mode :
         {SubassemblySolveMode::Rigid, SubassemblySolveMode::Flexible}) {
      OccurrenceState state;
      state.mobility = mobility;
      state.subassemblyMode = mode;
      assert(state.mobility == mobility && state.subassemblyMode == mode);
    }
  }

  const auto inserted = OccurrenceState::defaultOriginAligned();
  assert(inserted.localTransform == Transform{});
  assert(inserted.mobility == PlacementMobility::Fixed);
  assert(inserted.insertionMethod == InsertionMethod::OriginAligned);
  assert(inserted.dofState.state == ConstraintState::Fixed);
  auto moved = Transform{};
  moved.matrix[12] = 2.5;
  const auto interactive = OccurrenceState::interactive(moved);
  assert(interactive.mobility == PlacementMobility::Floating);
  assert(interactive.localTransform.matrix[12] == 2.5);

  LocalKinematicFrame frame;
  assert(frame.validate());
  frame.xAxis = frame.zAxis;
  assert(!frame.validate());

  GeometryDescriptor cylinder;
  cylinder.kind = GeometryKind::Cylinder;
  cylinder.origin = {1, 2, 3};
  cylinder.direction = {0, 0, 1};
  cylinder.radius = 0.025;
  cylinder.samples = {{1, 0, 0}, {0, 1, 0}};
  RelationEndpoint endpoint;
  endpoint.occurrenceId = inserted.id;
  endpoint.geometry = cylinder;

  AssemblyRelation relation;
  relation.type = RelationType::Concentric;
  relation.endpoints.push_back(endpoint);
  relation.parameters.emplace("offset_si", 0.0);
  relation.motion.semantic = MotionSemantic::Revolute;
  relation.motion.parameters.emplace("lower_limit_rad", -1.0);
  relation.analysisHint = AnalysisRelationHint{
      AnalysisHintKind::JointCandidate, "review in FEM environment", false};

  AssemblyRelationsSnapshot snapshot;
  PartDefinitionMetadata part;
  part.revisionHash = "part-revision";
  AssemblyDefinitionMetadata assembly;
  assembly.revisionHash = "assembly-revision";
  snapshot.parts.push_back(part);
  snapshot.assemblies.push_back(assembly);
  snapshot.occurrences.push_back(inserted);
  snapshot.relations.push_back(relation);
  const auto bytes = serialize(snapshot);
  const auto loaded = deserialize(bytes);
  assert(loaded && loaded.value() == snapshot);
  assert(loaded.value().relations.front().endpoints.front().geometry ==
         cylinder);
  assert(loaded.value().relations.front().motion.semantic ==
         MotionSemantic::Revolute);
  assert(loaded.value().relations.front().analysisHint->kind ==
         AnalysisHintKind::JointCandidate);
  assert(!loaded.value().relations.front().analysisHint->explicitlyAccepted);
  assert(!deserialize("bad-data"));
}

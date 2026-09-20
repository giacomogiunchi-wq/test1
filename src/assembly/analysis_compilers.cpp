#include "duomec/assembly/analysis_compilers.hpp"

#include <algorithm>

namespace duomec::assembly {
namespace {
bool inScope(const AssemblyRelation &relation, const AnalysisScope &scope) {
  return !relation.endpoints.empty() &&
         std::ranges::all_of(relation.endpoints, [&](const auto &endpoint) {
           return scope.occurrences.contains(endpoint.occurrenceId);
         });
}
KinematicEntityKind semanticKind(const AssemblyRelation &relation) {
  switch (relation.type) {
  case RelationType::Fixed:
    return KinematicEntityKind::FixedBody;
  case RelationType::Lock:
    return KinematicEntityKind::FixedJoint;
  case RelationType::Hinge:
    return KinematicEntityKind::Revolute;
  case RelationType::UniversalJoint:
    return KinematicEntityKind::Universal;
  case RelationType::Screw:
    return KinematicEntityKind::Helical;
  case RelationType::Gear:
    return KinematicEntityKind::Gear;
  case RelationType::RackPinion:
    return KinematicEntityKind::RackPinion;
  case RelationType::BeltChain:
    return KinematicEntityKind::BeltPulley;
  case RelationType::Path:
    return KinematicEntityKind::Trajectory;
  default:
    break;
  }
  switch (relation.motion.semantic) {
  case MotionSemantic::Fixed:
    return KinematicEntityKind::FixedJoint;
  case MotionSemantic::Revolute:
    return KinematicEntityKind::Revolute;
  case MotionSemantic::Prismatic:
    return KinematicEntityKind::Prismatic;
  case MotionSemantic::Universal:
    return KinematicEntityKind::Universal;
  case MotionSemantic::Screw:
    return KinematicEntityKind::Helical;
  case MotionSemantic::Gear:
    return KinematicEntityKind::Gear;
  case MotionSemantic::RackPinion:
    return KinematicEntityKind::RackPinion;
  case MotionSemantic::Path:
    return KinematicEntityKind::Trajectory;
  default:
    return KinematicEntityKind::GenericConstraint;
  }
}
bool sameBodies(const AssemblyRelation &a, const AssemblyRelation &b) {
  std::set<cad::OccurrenceId> lhs, rhs;
  for (const auto &endpoint : a.endpoints)
    lhs.insert(endpoint.occurrenceId);
  for (const auto &endpoint : b.endpoints)
    rhs.insert(endpoint.occurrenceId);
  return lhs == rhs;
}
} // namespace

core::Result<KinematicEntity> SemanticMultibodyRelationExporter::exportRelation(
    const AssemblyRelation &relation) const {
  if (relation.state == RelationState::Suppressed ||
      relation.state == RelationState::DanglingReference)
    return core::Result<KinematicEntity>::failure(
        {core::ErrorCode::invalid_argument,
         "suppressed or dangling relation cannot be exported",
         "motion export"});
  KinematicEntity result;
  result.source = relation.id;
  result.kind = semanticKind(relation);
  result.parameters = relation.parameters;
  for (const auto &endpoint : relation.endpoints) {
    if (!endpoint.localFrame.validate())
      return core::Result<KinematicEntity>::failure(
          {core::ErrorCode::invalid_argument, "invalid reaction frame",
           "motion export"});
    result.occurrences.push_back(endpoint.occurrenceId);
    result.reactionFrames.push_back(endpoint.localFrame);
  }
  return core::Result<KinematicEntity>::success(std::move(result));
}

core::Result<CompiledKinematicModel> OnDemandKinematicModelCompiler::compile(
    const AnalysisScope &scope,
    std::span<const ComponentOccurrence> occurrences,
    std::span<const AssemblyRelation> relations,
    const IMultibodyRelationExporter &exporter) const {
  CompiledKinematicModel result;
  result.sourceRevision = scope.revision;
  for (const auto &occurrence : occurrences)
    if (scope.occurrences.contains(occurrence.placement.id))
      result.bodies.insert(occurrence.placement.id);
  for (const auto &relation : relations) {
    if (!inScope(relation, scope) ||
        relation.state == RelationState::Suppressed)
      continue;
    // A confirmed high-level joint supersedes its coincident/concentric pair.
    if ((relation.type == RelationType::Coincident ||
         relation.type == RelationType::Concentric) &&
        std::ranges::any_of(relations, [&](const auto &candidate) {
          return candidate.type == RelationType::Hinge &&
                 inScope(candidate, scope) && sameBodies(candidate, relation);
        }))
      continue;
    auto exported = exporter.exportRelation(relation);
    if (!exported)
      return core::Result<CompiledKinematicModel>::failure(exported.error());
    result.entities.push_back(std::move(exported).value());
  }
  return core::Result<CompiledKinematicModel>::success(std::move(result));
}

std::optional<FemRelationCandidate>
FemRelationHintExtractor::extract(const AssemblyRelation &relation) const {
  if (relation.state == RelationState::Suppressed ||
      relation.state == RelationState::DanglingReference)
    return std::nullopt;
  FemRelationCandidate result;
  result.source = relation.id;
  result.endpoints = relation.endpoints;
  switch (relation.type) {
  case RelationType::Coincident:
    result.kind = FemCandidateKind::ContactOrBonded;
    break;
  case RelationType::Hinge:
  case RelationType::Concentric:
    result.kind = FemCandidateKind::JointConnectorOrBearing;
    break;
  case RelationType::Lock:
  case RelationType::Fixed:
    result.kind = FemCandidateKind::RigidOrBondedConnector;
    break;
  case RelationType::Frame:
    result.kind = FemCandidateKind::RemotePointOrConnectorFrame;
    break;
  default:
    if (!relation.analysisHint ||
        relation.analysisHint->kind == AnalysisHintKind::None)
      return std::nullopt;
    result.kind = FemCandidateKind::GenericInterface;
  }
  // CAD intent is never acceptance of a physical FEM condition.
  result.requiresReview = true;
  result.confirmed = false;
  return result;
}

core::Result<CompiledFemHints> OnDemandAssemblyToFemCompiler::compile(
    const AnalysisScope &scope, std::span<const AssemblyRelation> relations,
    const FemRelationHintExtractor &extractor,
    const IAnalysisGeometryResolver &resolver) const {
  CompiledFemHints result;
  result.sourceRevision = scope.revision;
  result.scope = scope.occurrences;
  for (const auto &relation : relations) {
    if (!inScope(relation, scope))
      continue;
    auto candidate = extractor.extract(relation);
    if (!candidate)
      continue;
    for (const auto &endpoint : candidate->endpoints) {
      auto geometry =
          resolver.resolve(endpoint.occurrenceId, endpoint.topologyReferenceId);
      if (!geometry)
        return core::Result<CompiledFemHints>::failure(geometry.error());
      candidate->resolvedGeometry.push_back(std::move(geometry).value());
    }
    result.candidates.push_back(std::move(*candidate));
  }
  return core::Result<CompiledFemHints>::success(std::move(result));
}

} // namespace duomec::assembly

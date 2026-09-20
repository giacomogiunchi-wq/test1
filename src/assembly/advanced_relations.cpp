#include "duomec/assembly/advanced_relations.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace duomec::assembly {
namespace {
core::Result<AssemblyRelation> invalid(std::string message) {
  return core::Result<AssemblyRelation>::failure(
      {core::ErrorCode::invalid_argument, std::move(message),
       "advanced assembly relation"});
}
AssemblyRelation relation(RelationType type,
                          std::vector<RelationEndpoint> endpoints) {
  AssemblyRelation value;
  value.type = type;
  value.endpoints = std::move(endpoints);
  return value;
}
std::string endpointKey(const AssemblyRelation &relation) {
  std::vector<std::string> values;
  for (const auto &endpoint : relation.endpoints)
    values.push_back(endpoint.occurrenceId.value() + ":" +
                     endpoint.topologyReferenceId.value());
  std::sort(values.begin(), values.end());
  std::ostringstream out;
  out << static_cast<int>(relation.type);
  for (const auto &value : values)
    out << '|' << value;
  return out.str();
}
std::string parameterKey(const AssemblyRelation &relation) {
  std::ostringstream out;
  for (const auto &[name, value] : relation.parameters) {
    out << name << '=' << value.index() << ':';
    std::visit([&](const auto &item) { out << item; }, value);
    out << ';';
  }
  return out.str();
}
bool sameOccurrencePair(const AssemblyRelation &a, const AssemblyRelation &b) {
  std::set<cad::OccurrenceId> first, second;
  for (const auto &endpoint : a.endpoints)
    first.insert(endpoint.occurrenceId);
  for (const auto &endpoint : b.endpoints)
    second.insert(endpoint.occurrenceId);
  return first == second;
}
} // namespace

core::Result<AssemblyRelation>
makeLimitDistance(RelationEndpoint first, RelationEndpoint second,
                  const LimitDistanceDefinition &definition) {
  if (definition.minimum > definition.maximum ||
      definition.current < definition.minimum ||
      definition.current > definition.maximum)
    return invalid("distance limits or start value are invalid");
  first.localFrame = definition.directionFrame;
  auto value = relation(RelationType::LimitDistance,
                        {std::move(first), std::move(second)});
  value.parameters["minimum_si"] = definition.minimum;
  value.parameters["maximum_si"] = definition.maximum;
  value.parameters["current_si"] = definition.current;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation>
makeLimitAngle(RelationEndpoint first, RelationEndpoint second,
               const LimitAngleDefinition &definition) {
  if (definition.minimumRadians > definition.maximumRadians ||
      definition.currentRadians < definition.minimumRadians ||
      definition.currentRadians > definition.maximumRadians)
    return invalid("angle limits or start value are invalid");
  first.localFrame = definition.angularFrame;
  auto value =
      relation(RelationType::LimitAngle, {std::move(first), std::move(second)});
  value.parameters["minimum_rad"] = definition.minimumRadians;
  value.parameters["maximum_rad"] = definition.maximumRadians;
  value.parameters["current_rad"] = definition.currentRadians;
  value.parameters["alignment"] = static_cast<std::int64_t>(definition.branch);
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation>
makeLinearCoupler(RelationEndpoint first, RelationEndpoint second,
                  const LinearCouplerDefinition &definition) {
  if (definition.ratio == 0)
    return invalid("linear coupler ratio cannot be zero");
  first.localFrame = definition.firstFrame;
  second.localFrame = definition.secondFrame;
  auto value = relation(RelationType::LinearCoupler,
                        {std::move(first), std::move(second)});
  value.parameters["ratio"] = definition.ratio;
  value.parameters["offset_si"] = definition.offset;
  value.parameters["direction"] =
      static_cast<std::int64_t>(definition.direction);
  value.motion.semantic = MotionSemantic::Prismatic;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation>
makePathRelation(RelationEndpoint follower, RelationEndpoint path,
                 const PathRelationDefinition &definition) {
  if (path.geometry.kind != GeometryKind::Path &&
      path.geometry.kind != GeometryKind::Curve)
    return invalid("path relation requires a persistent path or curve");
  if (definition.positionMode == PathPositionMode::Percent &&
      (definition.value < 0 || definition.value > 1))
    return invalid("path percent must be in [0,1]");
  auto value =
      relation(RelationType::Path, {std::move(follower), std::move(path)});
  value.parameters["position_mode"] =
      static_cast<std::int64_t>(definition.positionMode);
  value.parameters["orientation_mode"] =
      static_cast<std::int64_t>(definition.orientationMode);
  value.parameters["value"] = definition.value;
  value.parameters["path_parameter"] = definition.persistentParameter;
  value.motion.semantic = MotionSemantic::Path;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation>
makeProfileCenter(std::vector<RelationEndpoint> endpoints,
                  const ProfileCenterDefinition &definition) {
  if (endpoints.size() < 2)
    return invalid("profile center requires two profiles");
  auto value = relation(RelationType::ProfileCenter, std::move(endpoints));
  value.parameters["profile_shape"] =
      static_cast<std::int64_t>(definition.profile);
  value.parameters["alignment"] =
      static_cast<std::int64_t>(definition.alignment);
  value.parameters["orientation_rad"] = definition.orientationRadians;
  value.parameters["offset_si"] = definition.offset;
  value.parameters["lock_rotation"] = definition.lockRotation;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
AssemblyRelation makeSymmetric(RelationEndpoint first, RelationEndpoint second,
                               RelationEndpoint plane) {
  return relation(RelationType::Symmetric,
                  {std::move(first), std::move(second), std::move(plane)});
}
core::Result<AssemblyRelation>
makeWidth(std::vector<RelationEndpoint> endpoints,
          const WidthDefinition &definition) {
  if (endpoints.size() < 4)
    return invalid("width requires two width and two tab references");
  if (definition.mode == WidthMode::Percent &&
      (definition.value < 0 || definition.value > 1))
    return invalid("width percent must be in [0,1]");
  auto value = relation(RelationType::Width, std::move(endpoints));
  value.parameters["mode"] = static_cast<std::int64_t>(definition.mode);
  value.parameters["value"] = definition.value;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
AssemblyRelation makeHinge(RelationEndpoint axis, RelationEndpoint seat) {
  auto value =
      relation(RelationType::Hinge, {std::move(axis), std::move(seat)});
  value.motion.semantic = MotionSemantic::Revolute;
  return value;
}
core::Result<AssemblyRelation> makeGear(RelationEndpoint first,
                                        RelationEndpoint second,
                                        const GearDefinition &definition) {
  if (definition.ratio == 0)
    return invalid("gear ratio cannot be zero");
  auto value =
      relation(RelationType::Gear, {std::move(first), std::move(second)});
  value.parameters["ratio"] = definition.ratio;
  value.parameters["direction"] =
      static_cast<std::int64_t>(definition.direction);
  value.parameters["phase_rad"] = definition.phaseRadians;
  value.motion.semantic = MotionSemantic::Gear;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation>
makeRackPinion(RelationEndpoint rack, RelationEndpoint pinion,
               const RackPinionDefinition &definition) {
  if (definition.pitchRadius <= 0 && definition.travelPerRevolution <= 0)
    return invalid("rack and pinion requires pitch radius or travel");
  auto value =
      relation(RelationType::RackPinion, {std::move(rack), std::move(pinion)});
  value.parameters["pitch_radius_si"] = definition.pitchRadius;
  value.parameters["travel_per_revolution_si"] = definition.travelPerRevolution;
  value.parameters["direction"] =
      static_cast<std::int64_t>(definition.direction);
  value.parameters["offset_si"] = definition.offset;
  value.motion.semantic = MotionSemantic::RackPinion;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation> makeScrew(RelationEndpoint first,
                                         RelationEndpoint second,
                                         const ScrewDefinition &definition) {
  if (definition.lead <= 0)
    return invalid("screw lead must be positive");
  auto value =
      relation(RelationType::Screw, {std::move(first), std::move(second)});
  value.parameters["lead_si"] = definition.lead;
  value.parameters["handedness"] =
      static_cast<std::int64_t>(definition.handedness);
  value.parameters["phase_rad"] = definition.phaseRadians;
  value.parameters["offset_si"] = definition.offset;
  value.motion.semantic = MotionSemantic::Screw;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
core::Result<AssemblyRelation> makeSlot(RelationEndpoint follower,
                                        RelationEndpoint slot,
                                        const SlotDefinition &definition) {
  if (slot.geometry.kind != GeometryKind::Slot)
    return invalid("slot relation requires slot geometry");
  if (definition.mode == SlotMode::Percent &&
      (definition.value < 0 || definition.value > 1))
    return invalid("slot percent must be in [0,1]");
  auto value =
      relation(RelationType::Slot, {std::move(follower), std::move(slot)});
  value.parameters["mode"] = static_cast<std::int64_t>(definition.mode);
  value.parameters["orientation"] =
      static_cast<std::int64_t>(definition.orientation);
  value.parameters["value"] = definition.value;
  return core::Result<AssemblyRelation>::success(std::move(value));
}
AssemblyRelation
makeUniversalJoint(RelationEndpoint input, RelationEndpoint output,
                   const UniversalJointDefinition &definition) {
  auto value = relation(RelationType::UniversalJoint,
                        {std::move(input), std::move(output)});
  value.parameters["center_x"] = definition.center.x;
  value.parameters["center_y"] = definition.center.y;
  value.parameters["center_z"] = definition.center.z;
  value.parameters["phase_rad"] = definition.phaseRadians;
  value.motion.semantic = MotionSemantic::Universal;
  return value;
}
AssemblyRelation makeCam(RelationEndpoint cam, RelationEndpoint follower,
                         const CamDefinition &definition) {
  auto value =
      relation(RelationType::Cam, {std::move(cam), std::move(follower)});
  value.parameters["contact_side"] =
      static_cast<std::int64_t>(definition.contactSide);
  value.parameters["follower_geometry"] =
      static_cast<std::int64_t>(definition.follower);
  value.motion.semantic = MotionSemantic::GenericConstraint;
  return value;
}
core::Result<AssemblyRelation>
makeBeltChain(std::vector<RelationEndpoint> axes,
              const BeltChainDefinition &definition) {
  if (axes.size() < 2 || definition.effectiveRadii.size() != axes.size() ||
      std::any_of(definition.effectiveRadii.begin(),
                  definition.effectiveRadii.end(),
                  [](double value) { return value <= 0; }))
    return invalid("belt/chain requires matching positive pulley radii");
  auto value = relation(RelationType::BeltChain, std::move(axes));
  value.parameters["wrap"] = static_cast<std::int64_t>(definition.wrap);
  value.parameters["phase_rad"] = definition.phaseRadians;
  value.parameters["belt_length_si"] = definition.beltLength;
  for (std::size_t i = 0; i < definition.effectiveRadii.size(); ++i)
    value.parameters["radius_" + std::to_string(i) + "_si"] =
        definition.effectiveRadii[i];
  value.motion.semantic = MotionSemantic::Gear;
  return core::Result<AssemblyRelation>::success(std::move(value));
}

std::size_t expectedRemainingDofs(const AssemblyRelation &value) {
  switch (value.type) {
  case RelationType::Hinge:
  case RelationType::Screw:
  case RelationType::Slot:
  case RelationType::Path:
  case RelationType::ProfileCenter:
    return 1;
  case RelationType::UniversalJoint:
    return 2;
  case RelationType::Lock:
  case RelationType::Fixed:
  case RelationType::Frame:
    return 0;
  default:
    return 5;
  }
}

std::vector<RelationConflict> RelationDiagnosticService::localize(
    std::span<const AssemblyRelation> relations) const {
  std::map<std::string, std::vector<const AssemblyRelation *>> groups;
  for (const auto &value : relations)
    if (value.state != RelationState::Suppressed)
      groups[endpointKey(value)].push_back(&value);
  std::vector<RelationConflict> result;
  for (const auto &[key, values] : groups) {
    (void)key;
    for (std::size_t i = 0; i < values.size(); ++i)
      for (std::size_t j = i + 1; j < values.size(); ++j)
        if (parameterKey(*values[i]) != parameterKey(*values[j]))
          result.push_back(
              {{values[i]->id, values[j]->id},
               "same semantic endpoints have incompatible parameters"});
  }
  return result;
}
std::map<cad::AssemblyRelationId, RelationState>
RelationDiagnosticService::classify(std::span<const AssemblyRelation> relations,
                                    const SolveResult &solve) const {
  std::map<cad::AssemblyRelationId, RelationState> result;
  std::set<std::string> seen;
  const auto conflicts = localize(relations);
  std::set<cad::AssemblyRelationId> conflictIds;
  for (const auto &conflict : conflicts)
    conflictIds.insert(conflict.relations.begin(), conflict.relations.end());
  for (const auto &value : relations) {
    if (value.state == RelationState::Suppressed ||
        value.state == RelationState::DanglingReference ||
        value.state == RelationState::NeedsReview) {
      result[value.id] = value.state;
      continue;
    }
    if (conflictIds.contains(value.id))
      result[value.id] = RelationState::Conflicting;
    else if (!seen.insert(endpointKey(value) + parameterKey(value)).second)
      result[value.id] = RelationState::Redundant;
    else if (solve.status == SolveStatus::IterationLimit ||
             solve.status == SolveStatus::Cancelled)
      result[value.id] = RelationState::SolverFailed;
    else if (solve.status == SolveStatus::UnderConstrained)
      result[value.id] = RelationState::Underconstrained;
    else
      result[value.id] = RelationState::Solved;
  }
  return result;
}

ReferenceRepairPreview RelationReferenceRepairService::preview(
    const RelationEndpoint &endpoint,
    const DefinitionReferenceCatalog &oldCatalog,
    const DefinitionReferenceCatalog &newCatalog,
    const ExactResolver &exactResolver) const {
  if (const auto exact = exactResolver(endpoint.topologyReferenceId))
    return {RepairResolution::Exact,
            {{*exact, ReferenceMappingStrategy::StableNamedReference}}};
  ReferenceRepairPreview result;
  if (const auto old =
          oldCatalog.descriptors.find(endpoint.topologyReferenceId);
      old != oldCatalog.descriptors.end())
    for (const auto &[id, descriptor] : newCatalog.descriptors)
      if (descriptor.kind == old->second.kind)
        result.candidates.push_back(
            {id, ReferenceMappingStrategy::CompatibleDescriptor});
  if (result.candidates.empty()) {
    if (const auto old =
            oldCatalog.topologySignatures.find(endpoint.topologyReferenceId);
        old != oldCatalog.topologySignatures.end())
      for (const auto &[id, signature] : newCatalog.topologySignatures)
        if (signature == old->second)
          result.candidates.push_back(
              {id, ReferenceMappingStrategy::TopologySignature});
  }
  if (result.candidates.size() == 1)
    result.resolution = RepairResolution::Fallback;
  else if (result.candidates.size() > 1)
    result.resolution = RepairResolution::Ambiguous;
  return result;
}
core::Result<bool> RelationReferenceRepairService::apply(
    AssemblyRelation &relationValue, cad::RelationEndpointId endpointId,
    const ReferenceRepairPreview &preview,
    std::optional<std::size_t> selected) const {
  auto endpoint = std::find_if(
      relationValue.endpoints.begin(), relationValue.endpoints.end(),
      [&](const auto &value) { return value.id == endpointId; });
  if (endpoint == relationValue.endpoints.end())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "relation endpoint missing",
                                        endpointId.value()});
  std::size_t index = 0;
  if (preview.resolution == RepairResolution::Ambiguous) {
    if (!selected || *selected >= preview.candidates.size()) {
      relationValue.state = RelationState::NeedsReview;
      return core::Result<bool>::failure(
          {core::ErrorCode::invalid_argument,
           "ambiguous repair requires user selection",
           relationValue.id.value()});
    }
    index = *selected;
  } else if (preview.candidates.size() != 1) {
    relationValue.state = RelationState::DanglingReference;
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "reference remains unresolved",
                                        relationValue.id.value()});
  }
  endpoint->topologyReferenceId = preview.candidates[index].reference;
  relationValue.state = preview.resolution == RepairResolution::Exact
                            ? RelationState::Active
                            : RelationState::NeedsReview;
  return core::Result<bool>::success(true);
}

RelationView
RelationViewService::build(std::span<const cad::OccurrenceId> selected,
                           std::span<const ComponentOccurrence> occurrences,
                           std::span<const AssemblyRelation> relations,
                           const AssemblyRelationGraph &graph,
                           bool fadeUnrelated, bool isolateIsland) const {
  RelationView result;
  for (const auto &value : relations) {
    const bool related =
        std::any_of(value.endpoints.begin(), value.endpoints.end(),
                    [&](const auto &endpoint) {
                      return std::find(selected.begin(), selected.end(),
                                       endpoint.occurrenceId) != selected.end();
                    });
    if (!related)
      continue;
    result.relations.push_back(value.id);
    for (const auto &endpoint : value.endpoints)
      result.relatedOccurrences.insert(endpoint.occurrenceId);
  }
  if (fadeUnrelated)
    for (const auto &occurrence : occurrences)
      if (!result.relatedOccurrences.contains(occurrence.placement.id))
        result.fadedOccurrences.insert(occurrence.placement.id);
  if (isolateIsland && !selected.empty())
    result.isolatedIsland = graph.islandFor(selected.front());
  return result;
}

RelationBrowserGroups
RelationBrowserModel::group(std::span<const AssemblyRelation> relations,
                            RelationBrowserGrouping grouping,
                            const AssemblyRelationGraph *graph) const {
  RelationBrowserGroups result;
  for (const auto &value : relations) {
    std::string key;
    switch (grouping) {
    case RelationBrowserGrouping::Component:
      key = value.endpoints.empty()
                ? "unbound"
                : value.endpoints.front().occurrenceId.value();
      break;
    case RelationBrowserGrouping::Status:
      key = std::to_string(static_cast<int>(value.state));
      break;
    case RelationBrowserGrouping::Mechanical:
      key = value.type >= RelationType::Cam ? "mechanical" : "geometric";
      break;
    case RelationBrowserGrouping::UserFolder:
      if (const auto found = value.parameters.find("folder");
          found != value.parameters.end() &&
          std::holds_alternative<std::string>(found->second))
        key = std::get<std::string>(found->second);
      else
        key = "Unfiled";
      break;
    case RelationBrowserGrouping::SolveIsland:
      key = "unassigned";
      if (graph && !value.endpoints.empty())
        if (const auto island =
                graph->islandFor(value.endpoints.front().occurrenceId))
          key = island->id.value();
      break;
    }
    result[key].push_back(value.id);
  }
  return result;
}

std::vector<KinematicJointCandidate> KinematicJointRecognizer::recognize(
    std::span<const AssemblyRelation> relations) const {
  std::vector<KinematicJointCandidate> result;
  for (std::size_t i = 0; i < relations.size(); ++i) {
    if (relations[i].type == RelationType::Path) {
      MotionSemanticDescriptor motion;
      motion.semantic = MotionSemantic::Prismatic;
      result.push_back(
          {JointCandidateKind::Prismatic, {relations[i].id}, motion});
    }
    const bool spherical = std::any_of(
        relations[i].endpoints.begin(), relations[i].endpoints.end(),
        [](const auto &e) { return e.geometry.kind == GeometryKind::Sphere; });
    if (spherical) {
      MotionSemanticDescriptor motion;
      motion.semantic = MotionSemantic::Spherical;
      result.push_back(
          {JointCandidateKind::Spherical, {relations[i].id}, motion});
    }
    for (std::size_t j = i + 1; j < relations.size(); ++j)
      if (sameOccurrencePair(relations[i], relations[j]) &&
          ((relations[i].type == RelationType::Concentric &&
            relations[j].type == RelationType::Coincident) ||
           (relations[j].type == RelationType::Concentric &&
            relations[i].type == RelationType::Coincident))) {
        MotionSemanticDescriptor motion;
        motion.semantic = MotionSemantic::Revolute;
        result.push_back({JointCandidateKind::Revolute,
                          {relations[i].id, relations[j].id},
                          motion});
      }
  }
  return result;
}
core::Result<AssemblyRelation> KinematicJointComposer::compose(
    const KinematicJointCandidate &candidate,
    std::span<const AssemblyRelation> sourceRelations, bool confirmed) const {
  if (!confirmed)
    return invalid("joint composition requires user confirmation");
  std::vector<RelationEndpoint> endpoints;
  for (const auto &id : candidate.sourceRelations) {
    const auto found =
        std::find_if(sourceRelations.begin(), sourceRelations.end(),
                     [&](const auto &r) { return r.id == id; });
    if (found == sourceRelations.end())
      return invalid("joint source relation missing");
    for (const auto &e : found->endpoints)
      if (std::none_of(endpoints.begin(), endpoints.end(), [&](const auto &v) {
            return v.occurrenceId == e.occurrenceId;
          }))
        endpoints.push_back(e);
  }
  auto value = relation(candidate.kind == JointCandidateKind::Revolute
                            ? RelationType::Hinge
                            : RelationType::Generic,
                        std::move(endpoints));
  value.motion = candidate.motion;
  value.parameters["composed_from_count"] =
      static_cast<std::int64_t>(candidate.sourceRelations.size());
  return core::Result<AssemblyRelation>::success(std::move(value));
}
} // namespace duomec::assembly

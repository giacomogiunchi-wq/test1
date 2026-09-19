#pragma once

#include "duomec/core/error.hpp"

#include <compare>
#include <string>
#include <string_view>

namespace duomec::cad {

template <class Tag> class PersistentId {
public:
  [[nodiscard]] static PersistentId generate();
  [[nodiscard]] static core::Result<PersistentId> parse(std::string_view text);
  [[nodiscard]] const std::string &value() const noexcept { return value_; }
  auto operator<=>(const PersistentId &) const = default;

private:
  explicit PersistentId(std::string value) : value_(std::move(value)) {}
  std::string value_;
};

struct DocumentIdTag;
struct BodyIdTag;
struct FeatureIdTag;
struct SketchIdTag;
struct SketchEntityIdTag;
struct ConstraintIdTag;
struct ParameterIdTag;
struct TopologyReferenceIdTag;
struct MeshBodyIdTag;
struct PointCloudBodyIdTag;
struct DiscreteFeatureIdTag;
struct DiscreteRegionIdTag;
struct AssemblyRelationIdTag;
struct RelationEndpointIdTag;
struct KinematicFrameIdTag;
struct SolveIslandIdTag;
struct PartDefinitionIdTag;
struct AssemblyDefinitionIdTag;
struct OccurrenceIdTag;

using DocumentId = PersistentId<DocumentIdTag>;
using BodyId = PersistentId<BodyIdTag>;
using FeatureId = PersistentId<FeatureIdTag>;
using SketchId = PersistentId<SketchIdTag>;
using SketchEntityId = PersistentId<SketchEntityIdTag>;
using ConstraintId = PersistentId<ConstraintIdTag>;
using ParameterId = PersistentId<ParameterIdTag>;
using TopologyReferenceId = PersistentId<TopologyReferenceIdTag>;
using MeshBodyId = PersistentId<MeshBodyIdTag>;
using PointCloudBodyId = PersistentId<PointCloudBodyIdTag>;
using DiscreteFeatureId = PersistentId<DiscreteFeatureIdTag>;
using DiscreteRegionId = PersistentId<DiscreteRegionIdTag>;
using AssemblyRelationId = PersistentId<AssemblyRelationIdTag>;
using RelationEndpointId = PersistentId<RelationEndpointIdTag>;
using KinematicFrameId = PersistentId<KinematicFrameIdTag>;
using SolveIslandId = PersistentId<SolveIslandIdTag>;
using PartDefinitionId = PersistentId<PartDefinitionIdTag>;
using AssemblyDefinitionId = PersistentId<AssemblyDefinitionIdTag>;
using OccurrenceId = PersistentId<OccurrenceIdTag>;

extern template class PersistentId<DocumentIdTag>;
extern template class PersistentId<BodyIdTag>;
extern template class PersistentId<FeatureIdTag>;
extern template class PersistentId<SketchIdTag>;
extern template class PersistentId<SketchEntityIdTag>;
extern template class PersistentId<ConstraintIdTag>;
extern template class PersistentId<ParameterIdTag>;
extern template class PersistentId<TopologyReferenceIdTag>;
extern template class PersistentId<MeshBodyIdTag>;
extern template class PersistentId<PointCloudBodyIdTag>;
extern template class PersistentId<DiscreteFeatureIdTag>;
extern template class PersistentId<DiscreteRegionIdTag>;
extern template class PersistentId<AssemblyRelationIdTag>;
extern template class PersistentId<RelationEndpointIdTag>;
extern template class PersistentId<KinematicFrameIdTag>;
extern template class PersistentId<SolveIslandIdTag>;
extern template class PersistentId<PartDefinitionIdTag>;
extern template class PersistentId<AssemblyDefinitionIdTag>;
extern template class PersistentId<OccurrenceIdTag>;

} // namespace duomec::cad

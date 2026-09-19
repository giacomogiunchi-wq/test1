#pragma once

#include "duomec/cad/document/ids.hpp"
#include "duomec/core/error.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace duomec::assembly {

struct Vector3 {
  double x{};
  double y{};
  double z{};
  auto operator<=>(const Vector3 &) const = default;
};
struct Transform {
  std::array<double, 16> matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  auto operator<=>(const Transform &) const = default;
};

enum class AbsoluteReferenceKind {
  Origin,
  FrontPlane,
  TopPlane,
  RightPlane,
  XAxis,
  YAxis,
  ZAxis,
  PrimaryCoordinateFrame
};
struct AbsoluteReference {
  AbsoluteReferenceKind kind{};
  cad::TopologyReferenceId referenceId{cad::TopologyReferenceId::generate()};
  auto operator<=>(const AbsoluteReference &) const = default;
};
struct LightweightReferenceMetadata {
  std::array<AbsoluteReference, 8> references;
  [[nodiscard]] static LightweightReferenceMetadata create();
  [[nodiscard]] const AbsoluteReference &at(AbsoluteReferenceKind kind) const;
  auto operator<=>(const LightweightReferenceMetadata &) const = default;
};

struct PartDefinitionMetadata {
  cad::PartDefinitionId id{cad::PartDefinitionId::generate()};
  std::string revisionHash;
  LightweightReferenceMetadata absoluteReferences{
      LightweightReferenceMetadata::create()};
  auto operator<=>(const PartDefinitionMetadata &) const = default;
};
struct AssemblyDefinitionMetadata {
  cad::AssemblyDefinitionId id{cad::AssemblyDefinitionId::generate()};
  std::string revisionHash;
  LightweightReferenceMetadata absoluteReferences{
      LightweightReferenceMetadata::create()};
  auto operator<=>(const AssemblyDefinitionMetadata &) const = default;
};

enum class PlacementMobility { Fixed, Floating };
enum class SubassemblySolveMode { Rigid, Flexible };
enum class InsertionMethod { OriginAligned, Interactive };

enum class Dof : std::uint8_t { Tx, Ty, Tz, Rx, Ry, Rz };
enum class ConstraintState {
  Fixed,
  FullyConstrained,
  UnderConstrained,
  Redundant,
  Conflicting
};
struct DofState {
  std::array<bool, 6> free{true, true, true, true, true, true};
  ConstraintState state{ConstraintState::UnderConstrained};
  [[nodiscard]] static DofState floating();
  [[nodiscard]] static DofState fixed();
  [[nodiscard]] std::size_t independentDofCount() const;
  [[nodiscard]] bool isFree(Dof dof) const;
  [[nodiscard]] bool isConstrained(Dof dof) const;
  auto operator<=>(const DofState &) const = default;
};

enum class GeometryKind {
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
  Slot,
  Unknown
};
struct GeometryDescriptor {
  GeometryKind kind{GeometryKind::Unknown};
  Vector3 origin;
  Vector3 direction{0, 0, 1};
  Vector3 secondaryDirection{1, 0, 0};
  double radius{};
  double secondaryRadius{};
  double angleRadians{};
  std::vector<Vector3> samples;
  auto operator<=>(const GeometryDescriptor &) const = default;
};

struct LocalKinematicFrame {
  cad::KinematicFrameId id{cad::KinematicFrameId::generate()};
  Vector3 origin;
  Vector3 xAxis{1, 0, 0};
  Vector3 yAxis{0, 1, 0};
  Vector3 zAxis{0, 0, 1};
  [[nodiscard]] core::Result<bool> validate(double tolerance = 1e-9) const;
  auto operator<=>(const LocalKinematicFrame &) const = default;
};

enum class MotionSemantic {
  None,
  Fixed,
  Revolute,
  Prismatic,
  Cylindrical,
  Spherical,
  Universal,
  Screw,
  Gear,
  RackPinion,
  Path,
  GenericConstraint
};
struct MotionSemanticDescriptor {
  MotionSemantic semantic{MotionSemantic::None};
  std::map<std::string, double, std::less<>> parameters;
  auto operator<=>(const MotionSemanticDescriptor &) const = default;
};
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
  AnalysisHintKind kind{AnalysisHintKind::None};
  std::string userNote;
  bool explicitlyAccepted{};
  auto operator<=>(const AnalysisRelationHint &) const = default;
};

enum class RelationType {
  Generic,
  Fixed,
  Coincident,
  Concentric,
  Parallel,
  Perpendicular,
  Distance,
  Angle
};
enum class RelationState {
  Active,
  Suppressed,
  Unresolved,
  Redundant,
  Conflicting
};
struct RelationEndpoint {
  cad::RelationEndpointId id{cad::RelationEndpointId::generate()};
  cad::OccurrenceId occurrenceId{cad::OccurrenceId::generate()};
  cad::TopologyReferenceId topologyReferenceId{
      cad::TopologyReferenceId::generate()};
  GeometryDescriptor geometry;
  LocalKinematicFrame localFrame;
  auto operator<=>(const RelationEndpoint &) const = default;
};
using RelationParameter = std::variant<bool, std::int64_t, double, std::string>;
struct AssemblyRelation {
  cad::AssemblyRelationId id{cad::AssemblyRelationId::generate()};
  RelationType type{RelationType::Generic};
  std::vector<RelationEndpoint> endpoints;
  std::map<std::string, RelationParameter, std::less<>> parameters;
  RelationState state{RelationState::Active};
  DofState dofState{DofState::floating()};
  std::optional<cad::SolveIslandId> solveIsland;
  MotionSemanticDescriptor motion;
  std::optional<AnalysisRelationHint> analysisHint;
  auto operator<=>(const AssemblyRelation &) const = default;
};

struct OccurrenceState {
  cad::OccurrenceId id{cad::OccurrenceId::generate()};
  Transform localTransform;
  PlacementMobility mobility{PlacementMobility::Floating};
  SubassemblySolveMode subassemblyMode{SubassemblySolveMode::Rigid};
  InsertionMethod insertionMethod{InsertionMethod::Interactive};
  DofState dofState{DofState::floating()};
  [[nodiscard]] static OccurrenceState defaultOriginAligned();
  [[nodiscard]] static OccurrenceState interactive(Transform transform);
  auto operator<=>(const OccurrenceState &) const = default;
};

struct AssemblyRelationsSnapshot {
  static constexpr std::uint32_t currentSchemaVersion = 1;
  std::uint32_t schemaVersion{currentSchemaVersion};
  std::vector<PartDefinitionMetadata> parts;
  std::vector<AssemblyDefinitionMetadata> assemblies;
  std::vector<OccurrenceState> occurrences;
  std::vector<AssemblyRelation> relations;
  auto operator<=>(const AssemblyRelationsSnapshot &) const = default;
};

[[nodiscard]] std::string serialize(const AssemblyRelationsSnapshot &snapshot);
[[nodiscard]] core::Result<AssemblyRelationsSnapshot>
deserialize(std::string_view bytes);

} // namespace duomec::assembly

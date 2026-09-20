#pragma once

#include "duomec/assembly/lifecycle.hpp"

#include <functional>

namespace duomec::assembly {

enum class DirectionSense { Positive, Negative };
enum class PathPositionMode { Free, Distance, Percent };
enum class PathOrientationMode { Free, FollowTangent };
enum class ProfileShape { Circular, Rectangular, RegularPolygon };
enum class WidthMode { Center, Free, Distance, Percent };
enum class SlotMode { Free, Center, Distance, Percent };
enum class Handedness { RightHanded, LeftHanded };
enum class ContactSide { Inside, Outside, Either };
enum class FollowerGeometry { Point, Roller, Flat };
enum class WrapOrientation { Open, Crossed };

struct LimitDistanceDefinition {
  double minimum{};
  double maximum{};
  double current{};
  LocalKinematicFrame directionFrame;
};
struct LimitAngleDefinition {
  double minimumRadians{};
  double maximumRadians{};
  double currentRadians{};
  LocalKinematicFrame angularFrame;
  Alignment branch{Alignment::Aligned};
};
struct LinearCouplerDefinition {
  double ratio{1};
  double offset{};
  DirectionSense direction{DirectionSense::Positive};
  LocalKinematicFrame firstFrame;
  LocalKinematicFrame secondFrame;
};
struct PathRelationDefinition {
  PathPositionMode positionMode{PathPositionMode::Free};
  PathOrientationMode orientationMode{PathOrientationMode::Free};
  double value{};
  double persistentParameter{};
};
struct ProfileCenterDefinition {
  ProfileShape profile{ProfileShape::Circular};
  Alignment alignment{Alignment::Aligned};
  double orientationRadians{};
  double offset{};
  bool lockRotation{};
};
struct WidthDefinition {
  WidthMode mode{WidthMode::Center};
  double value{};
};
struct GearDefinition {
  double ratio{1};
  DirectionSense direction{DirectionSense::Negative};
  double phaseRadians{};
};
struct RackPinionDefinition {
  double pitchRadius{};
  double travelPerRevolution{};
  DirectionSense direction{DirectionSense::Positive};
  double offset{};
};
struct ScrewDefinition {
  double lead{};
  Handedness handedness{Handedness::RightHanded};
  double phaseRadians{};
  double offset{};
};
struct SlotDefinition {
  SlotMode mode{SlotMode::Free};
  PathOrientationMode orientation{PathOrientationMode::Free};
  double value{};
};
struct UniversalJointDefinition {
  Vector3 center;
  double phaseRadians{};
};
struct CamDefinition {
  ContactSide contactSide{ContactSide::Either};
  FollowerGeometry follower{FollowerGeometry::Point};
};
struct BeltChainDefinition {
  std::vector<double> effectiveRadii;
  WrapOrientation wrap{WrapOrientation::Open};
  double phaseRadians{};
  double beltLength{};
};

core::Result<AssemblyRelation>
makeLimitDistance(RelationEndpoint first, RelationEndpoint second,
                  const LimitDistanceDefinition &definition);
core::Result<AssemblyRelation>
makeLimitAngle(RelationEndpoint first, RelationEndpoint second,
               const LimitAngleDefinition &definition);
core::Result<AssemblyRelation>
makeLinearCoupler(RelationEndpoint first, RelationEndpoint second,
                  const LinearCouplerDefinition &definition);
core::Result<AssemblyRelation>
makePathRelation(RelationEndpoint follower, RelationEndpoint path,
                 const PathRelationDefinition &definition);
core::Result<AssemblyRelation>
makeProfileCenter(std::vector<RelationEndpoint> endpoints,
                  const ProfileCenterDefinition &definition);
AssemblyRelation makeSymmetric(RelationEndpoint first, RelationEndpoint second,
                               RelationEndpoint symmetryPlane);
core::Result<AssemblyRelation>
makeWidth(std::vector<RelationEndpoint> widthAndTabReferences,
          const WidthDefinition &definition);
AssemblyRelation makeHinge(RelationEndpoint axis, RelationEndpoint seat);
core::Result<AssemblyRelation> makeGear(RelationEndpoint firstAxis,
                                        RelationEndpoint secondAxis,
                                        const GearDefinition &definition);
core::Result<AssemblyRelation>
makeRackPinion(RelationEndpoint rackAxis, RelationEndpoint pinionAxis,
               const RackPinionDefinition &definition);
core::Result<AssemblyRelation> makeScrew(RelationEndpoint first,
                                         RelationEndpoint second,
                                         const ScrewDefinition &definition);
core::Result<AssemblyRelation> makeSlot(RelationEndpoint follower,
                                        RelationEndpoint slot,
                                        const SlotDefinition &definition);
AssemblyRelation makeUniversalJoint(RelationEndpoint inputFrame,
                                    RelationEndpoint outputFrame,
                                    const UniversalJointDefinition &definition);
AssemblyRelation makeCam(RelationEndpoint cam, RelationEndpoint follower,
                         const CamDefinition &definition);
core::Result<AssemblyRelation>
makeBeltChain(std::vector<RelationEndpoint> axes,
              const BeltChainDefinition &definition);

[[nodiscard]] std::size_t
expectedRemainingDofs(const AssemblyRelation &relation);

struct RelationConflict {
  std::vector<cad::AssemblyRelationId> relations;
  std::string reason;
};
class RelationDiagnosticService {
public:
  [[nodiscard]] std::vector<RelationConflict>
  localize(std::span<const AssemblyRelation> relations) const;
  [[nodiscard]] std::map<cad::AssemblyRelationId, RelationState>
  classify(std::span<const AssemblyRelation> relations,
           const SolveResult &solveResult) const;
};

enum class RepairResolution { Exact, Fallback, Ambiguous, Unresolved };
struct ReferenceRepairCandidate {
  cad::TopologyReferenceId reference;
  ReferenceMappingStrategy strategy;
};
struct ReferenceRepairPreview {
  RepairResolution resolution{RepairResolution::Unresolved};
  std::vector<ReferenceRepairCandidate> candidates;
};
class RelationReferenceRepairService {
public:
  using ExactResolver = std::function<std::optional<cad::TopologyReferenceId>(
      const cad::TopologyReferenceId &)>;
  [[nodiscard]] ReferenceRepairPreview
  preview(const RelationEndpoint &endpoint,
          const DefinitionReferenceCatalog &oldCatalog,
          const DefinitionReferenceCatalog &newCatalog,
          const ExactResolver &exactResolver) const;
  core::Result<bool> apply(AssemblyRelation &relation,
                           cad::RelationEndpointId endpoint,
                           const ReferenceRepairPreview &preview,
                           std::optional<std::size_t> selectedCandidate) const;
};

struct RelationView {
  std::vector<cad::AssemblyRelationId> relations;
  std::set<cad::OccurrenceId> relatedOccurrences;
  std::set<cad::OccurrenceId> fadedOccurrences;
  std::optional<SolveIsland> isolatedIsland;
};
class RelationViewService {
public:
  [[nodiscard]] RelationView
  build(std::span<const cad::OccurrenceId> selected,
        std::span<const ComponentOccurrence> allOccurrences,
        std::span<const AssemblyRelation> relations,
        const AssemblyRelationGraph &graph, bool fadeUnrelated,
        bool isolateIsland) const;
};

enum class RelationBrowserGrouping {
  Component,
  Status,
  Mechanical,
  UserFolder,
  SolveIsland
};
using RelationBrowserGroups =
    std::map<std::string, std::vector<cad::AssemblyRelationId>, std::less<>>;
class RelationBrowserModel {
public:
  [[nodiscard]] RelationBrowserGroups
  group(std::span<const AssemblyRelation> relations,
        RelationBrowserGrouping grouping,
        const AssemblyRelationGraph *graph = nullptr) const;
};

enum class JointCandidateKind { Revolute, Prismatic, Spherical };
struct KinematicJointCandidate {
  JointCandidateKind kind{};
  std::vector<cad::AssemblyRelationId> sourceRelations;
  MotionSemanticDescriptor motion;
};
class KinematicJointRecognizer {
public:
  [[nodiscard]] std::vector<KinematicJointCandidate>
  recognize(std::span<const AssemblyRelation> relations) const;
};
class KinematicJointComposer {
public:
  [[nodiscard]] core::Result<AssemblyRelation>
  compose(const KinematicJointCandidate &candidate,
          std::span<const AssemblyRelation> sourceRelations,
          bool confirmed) const;
};

} // namespace duomec::assembly

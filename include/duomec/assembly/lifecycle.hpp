#pragma once

#include "duomec/assembly/constraint_runtime.hpp"
#include "duomec/assembly/runtime_model.hpp"

#include <filesystem>
#include <set>

namespace duomec::assembly {

enum class DefinitionKind { Part, Subassembly };
enum class DefinitionStorage { External, Virtual };
using DefinitionId =
    std::variant<cad::PartDefinitionId, cad::AssemblyDefinitionId>;

struct SharedAssetReferences {
  std::string exactGeometryHash;
  std::string displayMeshHash;
  std::string edgeCacheHash;
  std::string selectionCacheHash;
  auto operator<=>(const SharedAssetReferences &) const = default;
};

enum class MateReferenceRank { Primary, Secondary, Tertiary };
struct MateReferenceDefinition {
  std::string name;
  MateReferenceRank rank{MateReferenceRank::Primary};
  std::int32_t priority{};
  cad::TopologyReferenceId reference{cad::TopologyReferenceId::generate()};
  RelationType preferredRelation{RelationType::Coincident};
  Alignment alignment{Alignment::Aligned};
  LocalKinematicFrame localFrame;
  GeometryDescriptor geometry;
  auto operator<=>(const MateReferenceDefinition &) const = default;
};

struct DefinitionReferenceCatalog {
  LightweightReferenceMetadata absoluteReferences{
      LightweightReferenceMetadata::create()};
  std::vector<MateReferenceDefinition> mateReferences;
  std::map<std::string, cad::TopologyReferenceId, std::less<>> namedReferences;
  std::map<cad::TopologyReferenceId, GeometryDescriptor> descriptors;
  std::map<cad::TopologyReferenceId, std::string> topologySignatures;
  auto operator<=>(const DefinitionReferenceCatalog &) const = default;
};

struct EmbeddedAuthoringData {
  std::vector<std::string> featureHistory;
  std::vector<std::string> bodies;
  std::vector<std::string> materials;
  std::map<std::string, std::string, std::less<>> metadata;
  std::vector<cad::TopologyReferenceId> assemblyContextReferences;
  auto operator<=>(const EmbeddedAuthoringData &) const = default;
};

struct ComponentDefinition {
  DefinitionId id{cad::PartDefinitionId::generate()};
  DefinitionKind kind{DefinitionKind::Part};
  DefinitionStorage storage{DefinitionStorage::External};
  std::string revisionHash;
  std::filesystem::path externalLocator;
  SharedAssetReferences sharedAssets;
  DefinitionReferenceCatalog references;
  EmbeddedAuthoringData authoring;
  auto operator<=>(const ComponentDefinition &) const = default;
};

struct ComponentOccurrence {
  OccurrenceState placement;
  DefinitionId definition{cad::PartDefinitionId::generate()};
  std::optional<cad::OccurrenceId> parent;
  bool visible{true};
  bool suppressed{};
  std::map<std::string, std::string, std::less<>> appearanceOverrides;
  std::map<std::string, std::string, std::less<>> metadata;
  auto operator<=>(const ComponentOccurrence &) const = default;
};

struct AssemblyLifecycleSnapshot {
  static constexpr std::uint32_t currentSchemaVersion = 2;
  std::uint32_t schemaVersion{currentSchemaVersion};
  std::vector<ComponentDefinition> definitions;
  std::vector<ComponentOccurrence> occurrences;
  AssemblyRelationsSnapshot relations;
  std::map<cad::AssemblyRelationId, cad::AssemblyDefinitionId> relationOwners;
  auto operator<=>(const AssemblyLifecycleSnapshot &) const = default;
};

class InsertComponentCommand {
public:
  [[nodiscard]] std::vector<ComponentOccurrence>
  insert(std::span<const DefinitionId> definitions,
         const std::optional<cad::OccurrenceId> &parent,
         std::optional<Transform> explicitPose = std::nullopt) const;
};

enum class ReferenceMigrationState {
  Resolved,
  NeedsReview,
  DanglingReference,
  Incompatible
};
enum class ReferenceMappingStrategy {
  PublishedMateReference,
  AbsoluteReference,
  StableNamedReference,
  CompatibleDescriptor,
  TopologySignature,
  ManualRepair
};
struct ReferenceMappingResult {
  ReferenceMigrationState state{ReferenceMigrationState::DanglingReference};
  ReferenceMappingStrategy strategy{ReferenceMappingStrategy::ManualRepair};
  std::optional<cad::TopologyReferenceId> replacement;
};
class ReplacementReferenceMapper {
public:
  [[nodiscard]] ReferenceMappingResult
  map(const cad::TopologyReferenceId &source,
      const DefinitionReferenceCatalog &oldCatalog,
      const DefinitionReferenceCatalog &newCatalog) const;
};
struct ReplacementReport {
  std::vector<cad::OccurrenceId> replacedOccurrences;
  std::map<cad::AssemblyRelationId, ReferenceMigrationState> relationStates;
};
class ReplaceComponentCommand {
public:
  [[nodiscard]] core::Result<ReplacementReport>
  execute(AssemblyLifecycleSnapshot &snapshot,
          std::span<const cad::OccurrenceId> selected,
          const DefinitionId &replacement, bool replaceAllInstances,
          const ReplacementReferenceMapper &mapper = {}) const;
  [[nodiscard]] core::Result<std::vector<InvalidationEvent>>
  execute(AssemblyRuntimeGraph &graph,
          std::span<const cad::OccurrenceId> selected,
          const CommittedDefinitionRef &replacement,
          bool replaceAllInstances) const;
};

class VirtualComponentCommands {
public:
  [[nodiscard]] ComponentDefinition newPart(std::string name) const;
  [[nodiscard]] ComponentDefinition newSubassembly(std::string name) const;
  [[nodiscard]] core::Result<bool>
  saveExternally(AssemblyLifecycleSnapshot &snapshot,
                 const DefinitionId &definition,
                 const std::filesystem::path &path,
                 std::span<const DefinitionId> embeddedChildren = {}) const;
};

class FormSubassemblyCommand {
public:
  [[nodiscard]] core::Result<cad::OccurrenceId> execute(
      AssemblyLifecycleSnapshot &snapshot,
      std::span<const cad::OccurrenceId> selected, DefinitionStorage storage,
      std::optional<std::filesystem::path> externalPath = std::nullopt) const;
};

class MakeIndependentCommand {
public:
  [[nodiscard]] core::Result<std::vector<DefinitionId>>
  execute(AssemblyLifecycleSnapshot &snapshot,
          std::span<const cad::OccurrenceId> selected,
          DefinitionStorage targetStorage) const;
  [[nodiscard]] core::Result<std::vector<CommittedDefinitionRef>>
  execute(DefinitionRegistry &registry, AssemblyRuntimeGraph &graph,
          std::span<const cad::OccurrenceId> selected) const;
};

core::Result<bool> setPlacementMobility(AssemblyLifecycleSnapshot &snapshot,
                                        const cad::OccurrenceId &occurrence,
                                        PlacementMobility mobility);
core::Result<bool> setSubassemblySolveMode(AssemblyLifecycleSnapshot &snapshot,
                                           const cad::OccurrenceId &occurrence,
                                           SubassemblySolveMode mode);

struct SmartInsertionPreview {
  Transform snappedPose;
  std::vector<AssemblyRelation> proposedRelations;
  bool compatible{};
};
class SmartInsertionEngine {
public:
  [[nodiscard]] SmartInsertionPreview
  preview(const ComponentOccurrence &inserted,
          const ComponentDefinition &insertedDefinition,
          const RelationEndpoint &target,
          const MateReferenceDefinition &targetReference) const;
  core::Result<bool> commit(AssemblyLifecycleSnapshot &snapshot,
                            const cad::OccurrenceId &occurrence,
                            const SmartInsertionPreview &preview) const;
};

class AssemblyLifecycleHistory {
public:
  explicit AssemblyLifecycleHistory(AssemblyLifecycleSnapshot initial = {});
  void commit(AssemblyLifecycleSnapshot next);
  core::Result<bool> undo();
  core::Result<bool> redo();
  [[nodiscard]] const AssemblyLifecycleSnapshot &current() const noexcept;

private:
  AssemblyLifecycleSnapshot current_;
  std::vector<AssemblyLifecycleSnapshot> undo_;
  std::vector<AssemblyLifecycleSnapshot> redo_;
};

[[nodiscard]] std::string
serializeLifecycle(const AssemblyLifecycleSnapshot &snapshot);
[[nodiscard]] core::Result<AssemblyLifecycleSnapshot>
deserializeLifecycle(std::string_view bytes);

} // namespace duomec::assembly

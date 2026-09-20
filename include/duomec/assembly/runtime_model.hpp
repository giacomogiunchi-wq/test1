#pragma once

#include "duomec/assembly/constraint_runtime.hpp"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace duomec::assembly {

// Content identities are deliberately opaque at this layer. Producers decide
// how bytes are canonicalized; consumers compare the role-specific value.
template <class Role> class ContentHash {
public:
  explicit ContentHash(std::string value) : value_(std::move(value)) {}
  [[nodiscard]] const std::string &value() const noexcept { return value_; }
  auto operator<=>(const ContentHash &) const = default;

private:
  std::string value_;
};
struct AuthoringHashRole;
struct GeometryHashRole;
struct DisplayRelevantHashRole;
using AuthoringHash = ContentHash<AuthoringHashRole>;
using GeometryHash = ContentHash<GeometryHashRole>;
using DisplayRelevantHash = ContentHash<DisplayRelevantHashRole>;

struct BodyRevision {
  cad::BodyRevisionId id{cad::BodyRevisionId::generate()};
  GeometryHash geometryHash{""};
  std::map<std::string, std::string, std::less<>> metadata;
  auto operator<=>(const BodyRevision &) const = default;
};

struct RevisionMetadata {
  std::string label;
  std::map<std::string, std::string, std::less<>> properties;
  auto operator<=>(const RevisionMetadata &) const = default;
};

struct PartRevision {
  cad::PartRevisionId id{cad::PartRevisionId::generate()};
  cad::PartDefinitionId definitionId{cad::PartDefinitionId::generate()};
  AuthoringHash authoringHash{""};
  GeometryHash geometryHash{""};
  DisplayRelevantHash displayRelevantHash{""};
  RevisionMetadata metadata;
  std::vector<BodyRevision> bodies;
  auto operator<=>(const PartRevision &) const = default;
};

struct PartDefinition {
  cad::PartDefinitionId id{cad::PartDefinitionId::generate()};
  std::string name;
  std::optional<cad::PartRevisionId> defaultRevision;
  LightweightReferenceMetadata absoluteReferences{
      LightweightReferenceMetadata::create()};
  std::map<std::string, std::string, std::less<>> metadata;
  auto operator<=>(const PartDefinition &) const = default;
};

struct AssemblyRevision {
  cad::AssemblyRevisionId id{cad::AssemblyRevisionId::generate()};
  cad::AssemblyDefinitionId definitionId{cad::AssemblyDefinitionId::generate()};
  AuthoringHash authoringHash{""};
  RevisionMetadata metadata;
  auto operator<=>(const AssemblyRevision &) const = default;
};

struct AssemblyDefinition {
  cad::AssemblyDefinitionId id{cad::AssemblyDefinitionId::generate()};
  std::string name;
  std::optional<cad::AssemblyRevisionId> defaultRevision;
  LightweightReferenceMetadata absoluteReferences{
      LightweightReferenceMetadata::create()};
  std::map<std::string, std::string, std::less<>> metadata;
  auto operator<=>(const AssemblyDefinition &) const = default;
};

struct PartDefinitionRef {
  cad::PartDefinitionId definitionId{cad::PartDefinitionId::generate()};
  cad::PartRevisionId revisionId{cad::PartRevisionId::generate()};
  AuthoringHash authoringHash{""};
  GeometryHash geometryHash{""};
  DisplayRelevantHash displayRelevantHash{""};
  auto operator<=>(const PartDefinitionRef &) const = default;
};
struct AssemblyDefinitionRef {
  cad::AssemblyDefinitionId definitionId{cad::AssemblyDefinitionId::generate()};
  cad::AssemblyRevisionId revisionId{cad::AssemblyRevisionId::generate()};
  AuthoringHash authoringHash{""};
  auto operator<=>(const AssemblyDefinitionRef &) const = default;
};
using CommittedDefinitionRef =
    std::variant<PartDefinitionRef, AssemblyDefinitionRef>;

class DefinitionRegistry {
public:
  core::Result<bool> addPartDefinition(PartDefinition definition);
  core::Result<bool> addAssemblyDefinition(AssemblyDefinition definition);
  core::Result<std::shared_ptr<const PartRevision>>
  commitPartRevision(PartRevision revision);
  core::Result<std::shared_ptr<const AssemblyRevision>>
  commitAssemblyRevision(AssemblyRevision revision);
  [[nodiscard]] std::shared_ptr<const PartDefinition>
  partDefinition(const cad::PartDefinitionId &id) const;
  [[nodiscard]] std::shared_ptr<const AssemblyDefinition>
  assemblyDefinition(const cad::AssemblyDefinitionId &id) const;
  [[nodiscard]] std::shared_ptr<const PartRevision>
  partRevision(const cad::PartRevisionId &id) const;
  [[nodiscard]] std::shared_ptr<const AssemblyRevision>
  assemblyRevision(const cad::AssemblyRevisionId &id) const;
  [[nodiscard]] core::Result<PartDefinitionRef>
  reference(const cad::PartDefinitionId &definition,
            const cad::PartRevisionId &revision) const;
  [[nodiscard]] core::Result<AssemblyDefinitionRef>
  reference(const cad::AssemblyDefinitionId &definition,
            const cad::AssemblyRevisionId &revision) const;

private:
  std::map<cad::PartDefinitionId, std::shared_ptr<const PartDefinition>> parts_;
  std::map<cad::AssemblyDefinitionId, std::shared_ptr<const AssemblyDefinition>>
      assemblies_;
  std::map<cad::PartRevisionId, std::shared_ptr<const PartRevision>>
      partRevisions_;
  std::map<cad::AssemblyRevisionId, std::shared_ptr<const AssemblyRevision>>
      assemblyRevisions_;
};

enum class InvalidationDomain {
  Metadata,
  Transform,
  Visibility,
  Selection,
  Tessellation,
  ExactGeometry,
  Authoring,
  MassProperties,
  AssemblyRelation,
  AssemblyBounds
};
struct InvalidationEvent {
  std::set<InvalidationDomain> domains;
  std::optional<cad::OccurrenceId> occurrence;
  std::optional<cad::PartDefinitionId> partDefinition;
  [[nodiscard]] bool contains(InvalidationDomain domain) const {
    return domains.contains(domain);
  }
};

struct AssemblyOccurrence {
  cad::OccurrenceId id{cad::OccurrenceId::generate()};
  CommittedDefinitionRef reference;
  std::optional<cad::OccurrenceId> parent;
  Transform localTransform;
  bool visible{true};
  bool suppressed{};
  std::map<std::string, std::string, std::less<>> appearanceOverrides;
  std::map<std::string, std::string, std::less<>> metadata;
  PlacementMobility mobility{PlacementMobility::Fixed};
  SubassemblySolveMode subassemblyMode{SubassemblySolveMode::Rigid};
};

class AssemblyRuntimeGraph {
public:
  explicit AssemblyRuntimeGraph(
      std::shared_ptr<const DefinitionRegistry> registry)
      : registry_(std::move(registry)) {}
  core::Result<cad::OccurrenceId> addOccurrence(AssemblyOccurrence occurrence);
  core::Result<bool> removeOccurrence(const cad::OccurrenceId &id);
  core::Result<InvalidationEvent>
  updateOccurrenceTransform(const cad::OccurrenceId &id, Transform transform);
  core::Result<InvalidationEvent> setVisibility(const cad::OccurrenceId &id,
                                                bool visible);
  core::Result<InvalidationEvent>
  setAppearanceOverride(const cad::OccurrenceId &id, std::string key,
                        std::string value);
  core::Result<bool> reparent(const cad::OccurrenceId &id,
                              std::optional<cad::OccurrenceId> parent,
                              bool preserveWorldTransform = false);
  core::Result<InvalidationEvent>
  replaceReference(const cad::OccurrenceId &id,
                   CommittedDefinitionRef reference);
  [[nodiscard]] const AssemblyOccurrence *
  find(const cad::OccurrenceId &id) const;
  [[nodiscard]] std::optional<cad::OccurrenceId>
  parentOf(const cad::OccurrenceId &id) const;
  [[nodiscard]] std::vector<cad::OccurrenceId>
  childrenOf(const std::optional<cad::OccurrenceId> &parent) const;
  [[nodiscard]] core::Result<std::vector<cad::OccurrenceId>>
  pathTo(const cad::OccurrenceId &id) const;
  [[nodiscard]] core::Result<Transform>
  worldTransform(const cad::OccurrenceId &id) const;
  [[nodiscard]] const DefinitionRegistry &registry() const noexcept {
    return *registry_;
  }
  [[nodiscard]] std::size_t size() const noexcept {
    return occurrences_.size();
  }
  [[nodiscard]] AssemblyRelationsSnapshot &relations() noexcept {
    return relations_;
  }
  [[nodiscard]] const AssemblyRelationsSnapshot &relations() const noexcept {
    return relations_;
  }

private:
  [[nodiscard]] bool validReference(const CommittedDefinitionRef &ref) const;
  std::shared_ptr<const DefinitionRegistry> registry_;
  std::map<cad::OccurrenceId, AssemblyOccurrence> occurrences_;
  std::map<std::optional<cad::OccurrenceId>, std::set<cad::OccurrenceId>>
      children_;
  AssemblyRelationsSnapshot relations_;
};

} // namespace duomec::assembly

#pragma once

#include "duomec/assembly/advanced_relations.hpp"

#include <memory>
#include <span>

namespace duomec::assembly {

struct AnalysisScope {
  std::set<cad::OccurrenceId> occurrences;
  std::string revision;
  auto operator<=>(const AnalysisScope &) const = default;
};

enum class KinematicEntityKind {
  FixedBody,
  FixedJoint,
  Revolute,
  Prismatic,
  Universal,
  Helical,
  Gear,
  RackPinion,
  BeltPulley,
  Trajectory,
  GenericConstraint
};
struct KinematicEntity {
  cad::AssemblyRelationId source{cad::AssemblyRelationId::generate()};
  KinematicEntityKind kind{KinematicEntityKind::GenericConstraint};
  std::vector<cad::OccurrenceId> occurrences;
  std::vector<LocalKinematicFrame> reactionFrames;
  std::map<std::string, RelationParameter, std::less<>> parameters;
};
struct CompiledKinematicModel {
  std::string sourceRevision;
  std::set<cad::OccurrenceId> bodies;
  std::vector<KinematicEntity> entities;
};

class IMultibodyRelationExporter {
public:
  virtual ~IMultibodyRelationExporter() = default;
  [[nodiscard]] virtual core::Result<KinematicEntity>
  exportRelation(const AssemblyRelation &relation) const = 0;
};
class IKinematicModelCompiler {
public:
  virtual ~IKinematicModelCompiler() = default;
  [[nodiscard]] virtual core::Result<CompiledKinematicModel>
  compile(const AnalysisScope &scope,
          std::span<const ComponentOccurrence> occurrences,
          std::span<const AssemblyRelation> relations,
          const IMultibodyRelationExporter &exporter) const = 0;
};
class SemanticMultibodyRelationExporter final
    : public IMultibodyRelationExporter {
public:
  [[nodiscard]] core::Result<KinematicEntity>
  exportRelation(const AssemblyRelation &relation) const override;
};
class OnDemandKinematicModelCompiler final : public IKinematicModelCompiler {
public:
  [[nodiscard]] core::Result<CompiledKinematicModel>
  compile(const AnalysisScope &scope,
          std::span<const ComponentOccurrence> occurrences,
          std::span<const AssemblyRelation> relations,
          const IMultibodyRelationExporter &exporter) const override;
};

struct ResolvedAnalysisGeometry {
  cad::TopologyReferenceId reference;
  std::string opaqueHandle;
};
class IAnalysisGeometryResolver {
public:
  virtual ~IAnalysisGeometryResolver() = default;
  [[nodiscard]] virtual core::Result<ResolvedAnalysisGeometry>
  resolve(cad::OccurrenceId occurrence,
          cad::TopologyReferenceId reference) const = 0;
};
enum class FemCandidateKind {
  ContactOrBonded,
  JointConnectorOrBearing,
  RigidOrBondedConnector,
  RemotePointOrConnectorFrame,
  GenericInterface
};
struct FemRelationCandidate {
  cad::AssemblyRelationId source{cad::AssemblyRelationId::generate()};
  FemCandidateKind kind{FemCandidateKind::GenericInterface};
  std::vector<RelationEndpoint> endpoints;
  std::vector<ResolvedAnalysisGeometry> resolvedGeometry;
  bool requiresReview{true};
  bool confirmed{};
};
struct CompiledFemHints {
  std::string sourceRevision;
  std::set<cad::OccurrenceId> scope;
  std::vector<FemRelationCandidate> candidates;
};
class FemRelationHintExtractor {
public:
  [[nodiscard]] std::optional<FemRelationCandidate>
  extract(const AssemblyRelation &relation) const;
};
class IAssemblyToFemCompiler {
public:
  virtual ~IAssemblyToFemCompiler() = default;
  [[nodiscard]] virtual core::Result<CompiledFemHints>
  compile(const AnalysisScope &scope,
          std::span<const AssemblyRelation> relations,
          const FemRelationHintExtractor &extractor,
          const IAnalysisGeometryResolver &resolver) const = 0;
};
class OnDemandAssemblyToFemCompiler final : public IAssemblyToFemCompiler {
public:
  [[nodiscard]] core::Result<CompiledFemHints>
  compile(const AnalysisScope &scope,
          std::span<const AssemblyRelation> relations,
          const FemRelationHintExtractor &extractor,
          const IAnalysisGeometryResolver &resolver) const override;
};

template <class Model> class AnalysisModelCache {
public:
  [[nodiscard]] const Model *find(std::string_view key) const {
    const auto found = models_.find(std::string(key));
    return found == models_.end() ? nullptr : &found->second.model;
  }
  void store(std::string key, AnalysisScope scope, Model model) {
    models_[std::move(key)] = {std::move(scope), std::move(model)};
  }
  void invalidate(cad::OccurrenceId changed) {
    std::erase_if(models_, [&](const auto &entry) {
      return entry.second.scope.occurrences.contains(changed);
    });
  }
  [[nodiscard]] std::size_t size() const noexcept { return models_.size(); }

private:
  struct Entry {
    AnalysisScope scope;
    Model model;
  };
  std::map<std::string, Entry, std::less<>> models_;
};

} // namespace duomec::assembly

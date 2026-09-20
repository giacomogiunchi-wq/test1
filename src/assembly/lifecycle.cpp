#include "duomec/assembly/lifecycle.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace duomec::assembly {
namespace {
template <class Id> Id parseId(const std::string &text) {
  auto value = Id::parse(text);
  if (!value)
    throw std::runtime_error("invalid persistent id: " + text);
  return value.value();
}
std::string idText(const DefinitionId &id) {
  return std::visit([](const auto &value) { return value.value(); }, id);
}
DefinitionId parseDefinitionId(DefinitionKind kind, const std::string &text) {
  if (kind == DefinitionKind::Part)
    return parseId<cad::PartDefinitionId>(text);
  return parseId<cad::AssemblyDefinitionId>(text);
}
bool sameDefinition(const DefinitionId &a, const DefinitionId &b) {
  return a.index() == b.index() && idText(a) == idText(b);
}
ComponentDefinition *findDefinition(AssemblyLifecycleSnapshot &snapshot,
                                    const DefinitionId &id) {
  const auto found = std::find_if(
      snapshot.definitions.begin(), snapshot.definitions.end(),
      [&](const auto &value) { return sameDefinition(value.id, id); });
  return found == snapshot.definitions.end() ? nullptr : &*found;
}
ComponentOccurrence *findOccurrence(AssemblyLifecycleSnapshot &snapshot,
                                    const cad::OccurrenceId &id) {
  const auto found =
      std::find_if(snapshot.occurrences.begin(), snapshot.occurrences.end(),
                   [&](const auto &value) { return value.placement.id == id; });
  return found == snapshot.occurrences.end() ? nullptr : &*found;
}
int migrationRank(ReferenceMigrationState value) {
  switch (value) {
  case ReferenceMigrationState::Resolved:
    return 0;
  case ReferenceMigrationState::NeedsReview:
    return 1;
  case ReferenceMigrationState::DanglingReference:
    return 2;
  case ReferenceMigrationState::Incompatible:
    return 3;
  }
  return 3;
}
RelationState relationState(ReferenceMigrationState value) {
  switch (value) {
  case ReferenceMigrationState::Resolved:
    return RelationState::Active;
  case ReferenceMigrationState::NeedsReview:
    return RelationState::NeedsReview;
  case ReferenceMigrationState::DanglingReference:
    return RelationState::DanglingReference;
  case ReferenceMigrationState::Incompatible:
    return RelationState::Incompatible;
  }
  return RelationState::DanglingReference;
}
std::string hex(std::string_view bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string out;
  out.reserve(bytes.size() * 2);
  for (unsigned char value : bytes) {
    out.push_back(digits[value >> 4]);
    out.push_back(digits[value & 15]);
  }
  return out;
}
std::string unhex(std::string_view value) {
  if (value.size() % 2)
    throw std::runtime_error("invalid relation payload");
  auto digit = [](char c) -> unsigned {
    if (c >= '0' && c <= '9')
      return static_cast<unsigned>(c - '0');
    if (c >= 'a' && c <= 'f')
      return static_cast<unsigned>(c - 'a' + 10);
    throw std::runtime_error("invalid hex");
  };
  std::string out;
  out.reserve(value.size() / 2);
  for (std::size_t i = 0; i < value.size(); i += 2)
    out.push_back(
        static_cast<char>((digit(value[i]) << 4) | digit(value[i + 1])));
  return out;
}
void writeVector(std::ostream &out, Vector3 v) {
  out << ' ' << v.x << ' ' << v.y << ' ' << v.z;
}
Vector3 readVector(std::istream &in) {
  Vector3 v;
  in >> v.x >> v.y >> v.z;
  return v;
}
void writeGeometryDetails(std::ostream &out,
                          const GeometryDescriptor &geometry) {
  writeVector(out, geometry.secondaryDirection);
  out << ' ' << geometry.secondaryRadius << ' ' << geometry.angleRadians << ' '
      << geometry.samples.size();
  for (const auto &sample : geometry.samples)
    writeVector(out, sample);
}
void readGeometryDetails(std::istream &in, GeometryDescriptor &geometry) {
  geometry.secondaryDirection = readVector(in);
  std::size_t sampleCount{};
  in >> geometry.secondaryRadius >> geometry.angleRadians >> sampleCount;
  if (sampleCount > 1'000'000)
    throw std::runtime_error("unreasonable geometry sample count");
  geometry.samples.reserve(sampleCount);
  for (std::size_t i = 0; i < sampleCount; ++i)
    geometry.samples.push_back(readVector(in));
}
template <class Map>
void writeStringMap(std::ostream &out, const char *tag, const Map &map) {
  out << tag << ' ' << map.size() << '\n';
  for (const auto &[k, v] : map)
    out << std::quoted(k) << ' ' << std::quoted(v) << '\n';
}
std::size_t readSectionCount(std::istream &in, std::string_view expected) {
  std::string tag;
  std::size_t count{};
  if (!(in >> tag >> count) || tag != expected || count > 1'000'000)
    throw std::runtime_error("invalid or unreasonable " +
                             std::string(expected) + " section");
  return count;
}
void readStringMap(std::istream &in, std::string_view expected,
                   std::map<std::string, std::string, std::less<>> &map) {
  const auto n = readSectionCount(in, expected);
  for (std::size_t i = 0; i < n; ++i) {
    std::string k, v;
    in >> std::quoted(k) >> std::quoted(v);
    map.emplace(std::move(k), std::move(v));
  }
}
} // namespace

std::vector<ComponentOccurrence>
InsertComponentCommand::insert(std::span<const DefinitionId> definitions,
                               const std::optional<cad::OccurrenceId> &parent,
                               std::optional<Transform> explicitPose) const {
  std::vector<ComponentOccurrence> result;
  result.reserve(definitions.size());
  for (const auto &definition : definitions) {
    ComponentOccurrence occurrence;
    occurrence.definition = definition;
    occurrence.parent = parent;
    occurrence.placement = explicitPose
                               ? OccurrenceState::interactive(*explicitPose)
                               : OccurrenceState::defaultOriginAligned();
    result.push_back(std::move(occurrence));
  }
  return result;
}

ReferenceMappingResult ReplacementReferenceMapper::map(
    const cad::TopologyReferenceId &source,
    const DefinitionReferenceCatalog &oldCatalog,
    const DefinitionReferenceCatalog &newCatalog) const {
  for (const auto &oldRef : oldCatalog.mateReferences)
    if (oldRef.reference == source)
      for (const auto &newRef : newCatalog.mateReferences)
        if (newRef.name == oldRef.name &&
            newRef.geometry.kind == oldRef.geometry.kind)
          return {ReferenceMigrationState::Resolved,
                  ReferenceMappingStrategy::PublishedMateReference,
                  newRef.reference};
  for (std::size_t i = 0; i < oldCatalog.absoluteReferences.references.size();
       ++i)
    if (oldCatalog.absoluteReferences.references[i].referenceId == source)
      return {ReferenceMigrationState::Resolved,
              ReferenceMappingStrategy::AbsoluteReference,
              newCatalog.absoluteReferences.references[i].referenceId};
  for (const auto &[name, id] : oldCatalog.namedReferences)
    if (id == source)
      if (const auto found = newCatalog.namedReferences.find(name);
          found != newCatalog.namedReferences.end())
        return {ReferenceMigrationState::Resolved,
                ReferenceMappingStrategy::StableNamedReference, found->second};
  if (const auto descriptor = oldCatalog.descriptors.find(source);
      descriptor != oldCatalog.descriptors.end()) {
    for (const auto &[id, value] : newCatalog.descriptors)
      if (value.kind == descriptor->second.kind)
        return {ReferenceMigrationState::NeedsReview,
                ReferenceMappingStrategy::CompatibleDescriptor, id};
    if (!newCatalog.descriptors.empty())
      return {ReferenceMigrationState::Incompatible,
              ReferenceMappingStrategy::ManualRepair, std::nullopt};
  }
  if (const auto signature = oldCatalog.topologySignatures.find(source);
      signature != oldCatalog.topologySignatures.end())
    for (const auto &[id, value] : newCatalog.topologySignatures)
      if (value == signature->second)
        return {ReferenceMigrationState::NeedsReview,
                ReferenceMappingStrategy::TopologySignature, id};
  return {};
}

core::Result<ReplacementReport> ReplaceComponentCommand::execute(
    AssemblyLifecycleSnapshot &snapshot,
    std::span<const cad::OccurrenceId> selected,
    const DefinitionId &replacement, bool replaceAllInstances,
    const ReplacementReferenceMapper &mapper) const {
  const auto *newDefinition = findDefinition(snapshot, replacement);
  if (!newDefinition)
    return core::Result<ReplacementReport>::failure(
        {core::ErrorCode::invalid_argument, "replacement definition missing",
         "ReplaceComponentCommand"});
  std::vector<DefinitionId> originals;
  for (const auto &id : selected) {
    const auto *occurrence = findOccurrence(snapshot, id);
    if (!occurrence)
      return core::Result<ReplacementReport>::failure(
          {core::ErrorCode::invalid_argument, "selected occurrence missing",
           id.value()});
    originals.push_back(occurrence->definition);
  }
  for (const auto &definition : originals)
    if (!findDefinition(snapshot, definition))
      return core::Result<ReplacementReport>::failure(
          {core::ErrorCode::invalid_argument, "source definition missing",
           idText(definition)});
  ReplacementReport report;
  for (auto &occurrence : snapshot.occurrences) {
    bool chosen = std::find(selected.begin(), selected.end(),
                            occurrence.placement.id) != selected.end();
    if (replaceAllInstances && !chosen)
      chosen =
          std::any_of(originals.begin(), originals.end(), [&](const auto &id) {
            return sameDefinition(id, occurrence.definition);
          });
    if (!chosen)
      continue;
    const auto *oldDefinition = findDefinition(snapshot, occurrence.definition);
    if (!oldDefinition)
      return core::Result<ReplacementReport>::failure(
          {core::ErrorCode::internal, "source definition disappeared",
           idText(occurrence.definition)});
    for (auto &relation : snapshot.relations.relations) {
      ReferenceMigrationState state = ReferenceMigrationState::Resolved;
      bool affected = false;
      for (auto &endpoint : relation.endpoints)
        if (endpoint.occurrenceId == occurrence.placement.id) {
          affected = true;
          const auto mapped =
              mapper.map(endpoint.topologyReferenceId,
                         oldDefinition->references, newDefinition->references);
          state = migrationRank(mapped.state) > migrationRank(state)
                      ? mapped.state
                      : state;
          if (mapped.replacement)
            endpoint.topologyReferenceId = *mapped.replacement;
        }
      if (affected) {
        auto &stored = report.relationStates[relation.id];
        if (migrationRank(state) > migrationRank(stored))
          stored = state;
        relation.state = relationState(stored);
      }
    }
    occurrence.definition = replacement;
    report.replacedOccurrences.push_back(occurrence.placement.id);
  }
  return core::Result<ReplacementReport>::success(std::move(report));
}

ComponentDefinition VirtualComponentCommands::newPart(std::string name) const {
  ComponentDefinition value;
  value.kind = DefinitionKind::Part;
  value.storage = DefinitionStorage::Virtual;
  value.revisionHash = "virtual:" + idText(value.id);
  value.authoring.metadata["name"] = std::move(name);
  return value;
}
ComponentDefinition
VirtualComponentCommands::newSubassembly(std::string name) const {
  ComponentDefinition value;
  value.id = cad::AssemblyDefinitionId::generate();
  value.kind = DefinitionKind::Subassembly;
  value.storage = DefinitionStorage::Virtual;
  value.revisionHash = "virtual:" + idText(value.id);
  value.authoring.metadata["name"] = std::move(name);
  return value;
}
core::Result<bool> VirtualComponentCommands::saveExternally(
    AssemblyLifecycleSnapshot &snapshot, const DefinitionId &definition,
    const std::filesystem::path &path,
    std::span<const DefinitionId> children) const {
  auto *record = findDefinition(snapshot, definition);
  if (!record || record->storage != DefinitionStorage::Virtual)
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "virtual definition missing",
                                        "saveExternally"});
  const auto temp = path.string() + ".tmp";
  std::ofstream out(temp, std::ios::binary | std::ios::trunc);
  if (!out)
    return core::Result<bool>::failure({core::ErrorCode::io_failure,
                                        "cannot create external component",
                                        path.string()});
  AssemblyLifecycleSnapshot exportSnapshot;
  auto exported = *record;
  exported.storage = DefinitionStorage::External;
  exported.externalLocator = path;
  exportSnapshot.definitions.push_back(std::move(exported));
  for (const auto &child : children)
    if (const auto *value = findDefinition(snapshot, child))
      exportSnapshot.definitions.push_back(*value);
  const auto bytes = serializeLifecycle(exportSnapshot);
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  out.close();
  if (!out) {
    std::filesystem::remove(temp);
    return core::Result<bool>::failure({core::ErrorCode::io_failure,
                                        "cannot write external component",
                                        path.string()});
  }
  std::error_code error;
  std::filesystem::rename(temp, path, error);
  if (error) {
    std::filesystem::remove(temp);
    return core::Result<bool>::failure(
        {core::ErrorCode::io_failure, error.message(), path.string()});
  }
  record->storage = DefinitionStorage::External;
  record->externalLocator = path;
  return core::Result<bool>::success(true);
}

core::Result<cad::OccurrenceId> FormSubassemblyCommand::execute(
    AssemblyLifecycleSnapshot &snapshot,
    std::span<const cad::OccurrenceId> selected, DefinitionStorage storage,
    std::optional<std::filesystem::path> externalPath) const {
  if (selected.empty())
    return core::Result<cad::OccurrenceId>::failure(
        {core::ErrorCode::invalid_argument, "empty selection",
         "FormSubassemblyCommand"});
  if (storage == DefinitionStorage::External && !externalPath)
    return core::Result<cad::OccurrenceId>::failure(
        {core::ErrorCode::invalid_argument,
         "external subassembly requires a destination path",
         "FormSubassemblyCommand"});
  std::optional<cad::OccurrenceId> parent;
  std::set<cad::OccurrenceId> selectedSet;
  for (const auto &id : selected) {
    const auto *o = findOccurrence(snapshot, id);
    if (!o)
      return core::Result<cad::OccurrenceId>::failure(
          {core::ErrorCode::invalid_argument, "occurrence missing",
           id.value()});
    if (selectedSet.empty())
      parent = o->parent;
    else if (o->parent != parent)
      return core::Result<cad::OccurrenceId>::failure(
          {core::ErrorCode::invalid_argument, "occurrences must share parent",
           "FormSubassemblyCommand"});
    selectedSet.insert(id);
  }
  const auto backup = snapshot;
  VirtualComponentCommands factory;
  auto definition = factory.newSubassembly("Subassembly");
  const auto definitionId = std::get<cad::AssemblyDefinitionId>(definition.id);
  snapshot.definitions.push_back(definition);
  ComponentOccurrence container;
  container.definition = definition.id;
  container.parent = parent;
  container.placement = OccurrenceState::defaultOriginAligned();
  const auto containerId = container.placement.id;
  snapshot.occurrences.push_back(container);
  for (auto &occurrence : snapshot.occurrences)
    if (selectedSet.contains(occurrence.placement.id))
      occurrence.parent = containerId;
  for (const auto &relation : snapshot.relations.relations) {
    bool any = false, all = true;
    for (const auto &e : relation.endpoints) {
      const bool inside = selectedSet.contains(e.occurrenceId);
      any = any || inside;
      all = all && inside;
    }
    if (any && all)
      snapshot.relationOwners.insert_or_assign(relation.id, definitionId);
  }
  if (storage == DefinitionStorage::External) {
    const DefinitionId id = definitionId;
    auto saved = factory.saveExternally(snapshot, id, *externalPath);
    if (!saved) {
      snapshot = backup;
      return core::Result<cad::OccurrenceId>::failure(saved.error());
    }
  }
  return core::Result<cad::OccurrenceId>::success(containerId);
}

core::Result<std::vector<DefinitionId>>
MakeIndependentCommand::execute(AssemblyLifecycleSnapshot &snapshot,
                                std::span<const cad::OccurrenceId> selected,
                                DefinitionStorage storage) const {
  for (const auto &id : selected) {
    const auto *occurrence = findOccurrence(snapshot, id);
    if (!occurrence)
      return core::Result<std::vector<DefinitionId>>::failure(
          {core::ErrorCode::invalid_argument, "occurrence missing",
           id.value()});
    if (!findDefinition(snapshot, occurrence->definition))
      return core::Result<std::vector<DefinitionId>>::failure(
          {core::ErrorCode::invalid_argument, "source definition missing",
           idText(occurrence->definition)});
  }
  std::vector<DefinitionId> result;
  for (const auto &id : selected) {
    auto *occurrence = findOccurrence(snapshot, id);
    if (!occurrence)
      return core::Result<std::vector<DefinitionId>>::failure(
          {core::ErrorCode::invalid_argument, "occurrence missing",
           id.value()});
    const auto *source = findDefinition(snapshot, occurrence->definition);
    if (!source)
      return core::Result<std::vector<DefinitionId>>::failure(
          {core::ErrorCode::invalid_argument, "definition missing",
           id.value()});
    auto clone = *source;
    clone.id = source->kind == DefinitionKind::Part
                   ? DefinitionId(cad::PartDefinitionId::generate())
                   : DefinitionId(cad::AssemblyDefinitionId::generate());
    clone.storage = storage;
    if (storage == DefinitionStorage::Virtual)
      clone.externalLocator.clear();
    snapshot.definitions.push_back(clone);
    occurrence = findOccurrence(snapshot, id);
    occurrence->definition = clone.id;
    result.push_back(clone.id);
  }
  return core::Result<std::vector<DefinitionId>>::success(std::move(result));
}

core::Result<bool> setPlacementMobility(AssemblyLifecycleSnapshot &s,
                                        const cad::OccurrenceId &id,
                                        PlacementMobility mobility) {
  auto *o = findOccurrence(s, id);
  if (!o)
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument, "occurrence missing", id.value()});
  o->placement.mobility = mobility;
  o->placement.dofState = mobility == PlacementMobility::Fixed
                              ? DofState::fixed()
                              : DofState::floating();
  return core::Result<bool>::success(true);
}
core::Result<bool> setSubassemblySolveMode(AssemblyLifecycleSnapshot &s,
                                           const cad::OccurrenceId &id,
                                           SubassemblySolveMode mode) {
  auto *o = findOccurrence(s, id);
  if (!o)
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument, "occurrence missing", id.value()});
  o->placement.subassemblyMode = mode;
  return core::Result<bool>::success(true);
}

SmartInsertionPreview SmartInsertionEngine::preview(
    const ComponentOccurrence &inserted, const ComponentDefinition &definition,
    const RelationEndpoint &target,
    const MateReferenceDefinition &targetReference) const {
  SmartInsertionPreview result;
  const auto found = std::find_if(
      definition.references.mateReferences.begin(),
      definition.references.mateReferences.end(), [&](const auto &r) {
        return r.geometry.kind == targetReference.geometry.kind &&
               r.preferredRelation == targetReference.preferredRelation;
      });
  if (found == definition.references.mateReferences.end())
    return result;
  RelationEndpoint source;
  source.occurrenceId = inserted.placement.id;
  source.topologyReferenceId = found->reference;
  source.geometry = found->geometry;
  source.localFrame = found->localFrame;
  AssemblyRelation relation;
  relation.type = found->preferredRelation;
  relation.endpoints = {source, target};
  relation.parameters["alignment"] =
      static_cast<std::int64_t>(found->alignment);
  result.snappedPose = inserted.placement.localTransform;
  result.proposedRelations.push_back(std::move(relation));
  result.compatible = true;
  return result;
}
core::Result<bool>
SmartInsertionEngine::commit(AssemblyLifecycleSnapshot &s,
                             const cad::OccurrenceId &id,
                             const SmartInsertionPreview &p) const {
  if (!p.compatible)
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "smart insertion is incompatible",
                                        "SmartInsertionEngine"});
  auto *o = findOccurrence(s, id);
  if (!o)
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument, "occurrence missing", id.value()});
  o->placement.localTransform = p.snappedPose;
  o->placement.mobility = PlacementMobility::Floating;
  s.relations.relations.insert(s.relations.relations.end(),
                               p.proposedRelations.begin(),
                               p.proposedRelations.end());
  return core::Result<bool>::success(true);
}

AssemblyLifecycleHistory::AssemblyLifecycleHistory(
    AssemblyLifecycleSnapshot initial)
    : current_(std::move(initial)) {}
void AssemblyLifecycleHistory::commit(AssemblyLifecycleSnapshot next) {
  undo_.push_back(current_);
  current_ = std::move(next);
  redo_.clear();
}
core::Result<bool> AssemblyLifecycleHistory::undo() {
  if (undo_.empty())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "nothing to undo",
                                        "AssemblyLifecycleHistory"});
  redo_.push_back(current_);
  current_ = std::move(undo_.back());
  undo_.pop_back();
  return core::Result<bool>::success(true);
}
core::Result<bool> AssemblyLifecycleHistory::redo() {
  if (redo_.empty())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "nothing to redo",
                                        "AssemblyLifecycleHistory"});
  undo_.push_back(current_);
  current_ = std::move(redo_.back());
  redo_.pop_back();
  return core::Result<bool>::success(true);
}
const AssemblyLifecycleSnapshot &
AssemblyLifecycleHistory::current() const noexcept {
  return current_;
}

std::string serializeLifecycle(const AssemblyLifecycleSnapshot &s) {
  std::ostringstream out;
  out << std::setprecision(17) << "DUOMEC_LIFECYCLE " << s.schemaVersion << '\n'
      << "DEFINITIONS " << s.definitions.size() << '\n';
  for (const auto &d : s.definitions) {
    out << static_cast<int>(d.kind) << ' ' << static_cast<int>(d.storage) << ' '
        << std::quoted(idText(d.id)) << ' ' << std::quoted(d.revisionHash)
        << ' ' << std::quoted(d.externalLocator.string()) << ' '
        << std::quoted(d.sharedAssets.exactGeometryHash) << ' '
        << std::quoted(d.sharedAssets.displayMeshHash) << ' '
        << std::quoted(d.sharedAssets.edgeCacheHash) << ' '
        << std::quoted(d.sharedAssets.selectionCacheHash) << '\n';
    out << "ABS";
    for (const auto &r : d.references.absoluteReferences.references)
      out << ' ' << std::quoted(r.referenceId.value());
    out << '\n';
    out << "MATES " << d.references.mateReferences.size() << '\n';
    for (const auto &m : d.references.mateReferences) {
      out << std::quoted(m.name) << ' ' << static_cast<int>(m.rank) << ' '
          << m.priority << ' ' << std::quoted(m.reference.value()) << ' '
          << static_cast<int>(m.preferredRelation) << ' '
          << static_cast<int>(m.alignment) << ' '
          << std::quoted(m.localFrame.id.value());
      writeVector(out, m.localFrame.origin);
      writeVector(out, m.localFrame.xAxis);
      writeVector(out, m.localFrame.yAxis);
      writeVector(out, m.localFrame.zAxis);
      out << ' ' << static_cast<int>(m.geometry.kind);
      writeVector(out, m.geometry.origin);
      writeVector(out, m.geometry.direction);
      out << ' ' << m.geometry.radius;
      if (s.schemaVersion >= 2)
        writeGeometryDetails(out, m.geometry);
      out << '\n';
    }
    out << "NAMED " << d.references.namedReferences.size() << '\n';
    for (const auto &[name, id] : d.references.namedReferences)
      out << std::quoted(name) << ' ' << std::quoted(id.value()) << '\n';
    out << "DESCRIPTORS " << d.references.descriptors.size() << '\n';
    for (const auto &[id, g] : d.references.descriptors) {
      out << std::quoted(id.value()) << ' ' << static_cast<int>(g.kind);
      writeVector(out, g.origin);
      writeVector(out, g.direction);
      out << ' ' << g.radius;
      if (s.schemaVersion >= 2)
        writeGeometryDetails(out, g);
      out << '\n';
    }
    out << "SIGNATURES " << d.references.topologySignatures.size() << '\n';
    for (const auto &[id, signature] : d.references.topologySignatures)
      out << std::quoted(id.value()) << ' ' << std::quoted(signature) << '\n';
    out << "FEATURES " << d.authoring.featureHistory.size() << '\n';
    for (const auto &v : d.authoring.featureHistory)
      out << std::quoted(v) << '\n';
    out << "BODIES " << d.authoring.bodies.size() << '\n';
    for (const auto &v : d.authoring.bodies)
      out << std::quoted(v) << '\n';
    out << "MATERIALS " << d.authoring.materials.size() << '\n';
    for (const auto &v : d.authoring.materials)
      out << std::quoted(v) << '\n';
    writeStringMap(out, "METADATA", d.authoring.metadata);
    out << "CONTEXT " << d.authoring.assemblyContextReferences.size() << '\n';
    for (const auto &id : d.authoring.assemblyContextReferences)
      out << std::quoted(id.value()) << '\n';
  }
  out << "OCCURRENCES " << s.occurrences.size() << '\n';
  for (const auto &o : s.occurrences) {
    out << std::quoted(o.placement.id.value()) << ' ' << o.definition.index()
        << ' ' << std::quoted(idText(o.definition)) << ' '
        << o.parent.has_value();
    if (o.parent)
      out << ' ' << std::quoted(o.parent->value());
    out << ' ' << o.visible << ' ' << o.suppressed << ' '
        << static_cast<int>(o.placement.mobility) << ' '
        << static_cast<int>(o.placement.subassemblyMode) << ' '
        << static_cast<int>(o.placement.insertionMethod);
    for (double v : o.placement.localTransform.matrix)
      out << ' ' << v;
    out << '\n';
    writeStringMap(out, "APPEARANCE", o.appearanceOverrides);
    writeStringMap(out, "OCCMETA", o.metadata);
  }
  out << "RELATIONS " << std::quoted(hex(serialize(s.relations))) << '\n'
      << "OWNERS " << s.relationOwners.size() << '\n';
  for (const auto &[r, d] : s.relationOwners)
    out << std::quoted(r.value()) << ' ' << std::quoted(d.value()) << '\n';
  return out.str();
}

core::Result<AssemblyLifecycleSnapshot>
deserializeLifecycle(std::string_view bytes) {
  try {
    std::istringstream in{std::string(bytes)};
    std::string tag;
    AssemblyLifecycleSnapshot s;
    in >> tag >> s.schemaVersion;
    if (tag != "DUOMEC_LIFECYCLE" || s.schemaVersion == 0 ||
        s.schemaVersion > AssemblyLifecycleSnapshot::currentSchemaVersion)
      throw std::runtime_error("unsupported lifecycle schema");
    std::size_t count{};
    const auto definitionCount = readSectionCount(in, "DEFINITIONS");
    for (std::size_t i = 0; i < definitionCount; ++i) {
      ComponentDefinition d;
      int kind, storage;
      std::string id, locator;
      in >> kind >> storage >> std::quoted(id) >> std::quoted(d.revisionHash) >>
          std::quoted(locator) >>
          std::quoted(d.sharedAssets.exactGeometryHash) >>
          std::quoted(d.sharedAssets.displayMeshHash) >>
          std::quoted(d.sharedAssets.edgeCacheHash) >>
          std::quoted(d.sharedAssets.selectionCacheHash);
      d.kind = static_cast<DefinitionKind>(kind);
      d.storage = static_cast<DefinitionStorage>(storage);
      d.id = parseDefinitionId(d.kind, id);
      d.externalLocator = locator;
      in >> tag;
      if (tag != "ABS")
        throw std::runtime_error("absolute references missing");
      for (auto &r : d.references.absoluteReferences.references) {
        std::string value;
        in >> std::quoted(value);
        r.referenceId = parseId<cad::TopologyReferenceId>(value);
      }
      count = readSectionCount(in, "MATES");
      for (std::size_t j = 0; j < count; ++j) {
        MateReferenceDefinition m;
        int rank, relation, alignment, geometry;
        std::string ref, frame;
        in >> std::quoted(m.name) >> rank >> m.priority >> std::quoted(ref) >>
            relation >> alignment >> std::quoted(frame);
        m.rank = static_cast<MateReferenceRank>(rank);
        m.reference = parseId<cad::TopologyReferenceId>(ref);
        m.preferredRelation = static_cast<RelationType>(relation);
        m.alignment = static_cast<Alignment>(alignment);
        m.localFrame.id = parseId<cad::KinematicFrameId>(frame);
        m.localFrame.origin = readVector(in);
        m.localFrame.xAxis = readVector(in);
        m.localFrame.yAxis = readVector(in);
        m.localFrame.zAxis = readVector(in);
        in >> geometry;
        m.geometry.kind = static_cast<GeometryKind>(geometry);
        m.geometry.origin = readVector(in);
        m.geometry.direction = readVector(in);
        in >> m.geometry.radius;
        if (s.schemaVersion >= 2)
          readGeometryDetails(in, m.geometry);
        d.references.mateReferences.push_back(std::move(m));
      }
      count = readSectionCount(in, "NAMED");
      for (std::size_t j = 0; j < count; ++j) {
        std::string name, reference;
        in >> std::quoted(name) >> std::quoted(reference);
        d.references.namedReferences.emplace(
            std::move(name), parseId<cad::TopologyReferenceId>(reference));
      }
      count = readSectionCount(in, "DESCRIPTORS");
      for (std::size_t j = 0; j < count; ++j) {
        std::string value;
        int kindValue;
        GeometryDescriptor g;
        in >> std::quoted(value) >> kindValue;
        g.kind = static_cast<GeometryKind>(kindValue);
        g.origin = readVector(in);
        g.direction = readVector(in);
        in >> g.radius;
        if (s.schemaVersion >= 2)
          readGeometryDetails(in, g);
        d.references.descriptors.emplace(
            parseId<cad::TopologyReferenceId>(value), g);
      }
      count = readSectionCount(in, "SIGNATURES");
      for (std::size_t j = 0; j < count; ++j) {
        std::string reference, signature;
        in >> std::quoted(reference) >> std::quoted(signature);
        d.references.topologySignatures.emplace(
            parseId<cad::TopologyReferenceId>(reference), std::move(signature));
      }
      auto readStrings = [&](std::string_view expected, auto &values) {
        count = readSectionCount(in, expected);
        for (std::size_t j = 0; j < count; ++j) {
          std::string value;
          in >> std::quoted(value);
          values.push_back(std::move(value));
        }
      };
      readStrings("FEATURES", d.authoring.featureHistory);
      readStrings("BODIES", d.authoring.bodies);
      readStrings("MATERIALS", d.authoring.materials);
      readStringMap(in, "METADATA", d.authoring.metadata);
      count = readSectionCount(in, "CONTEXT");
      for (std::size_t j = 0; j < count; ++j) {
        std::string value;
        in >> std::quoted(value);
        d.authoring.assemblyContextReferences.push_back(
            parseId<cad::TopologyReferenceId>(value));
      }
      s.definitions.push_back(std::move(d));
    }
    const auto occurrenceCount = readSectionCount(in, "OCCURRENCES");
    for (std::size_t i = 0; i < occurrenceCount; ++i) {
      ComponentOccurrence o;
      std::string occurrence, definition, parent;
      std::size_t definitionIndex;
      bool hasParent;
      int mobility, mode, insertion;
      in >> std::quoted(occurrence) >> definitionIndex >>
          std::quoted(definition) >> hasParent;
      if (hasParent)
        in >> std::quoted(parent);
      in >> o.visible >> o.suppressed >> mobility >> mode >> insertion;
      o.placement.id = parseId<cad::OccurrenceId>(occurrence);
      o.definition =
          definitionIndex == 0
              ? DefinitionId(parseId<cad::PartDefinitionId>(definition))
              : DefinitionId(parseId<cad::AssemblyDefinitionId>(definition));
      if (hasParent)
        o.parent = parseId<cad::OccurrenceId>(parent);
      o.placement.mobility = static_cast<PlacementMobility>(mobility);
      o.placement.subassemblyMode = static_cast<SubassemblySolveMode>(mode);
      o.placement.insertionMethod = static_cast<InsertionMethod>(insertion);
      o.placement.dofState = o.placement.mobility == PlacementMobility::Fixed
                                 ? DofState::fixed()
                                 : DofState::floating();
      for (auto &v : o.placement.localTransform.matrix)
        in >> v;
      readStringMap(in, "APPEARANCE", o.appearanceOverrides);
      readStringMap(in, "OCCMETA", o.metadata);
      s.occurrences.push_back(std::move(o));
    }
    std::string payload;
    in >> tag >> std::quoted(payload);
    if (tag != "RELATIONS")
      throw std::runtime_error("relations section missing");
    auto relations = deserialize(unhex(payload));
    if (!relations)
      return core::Result<AssemblyLifecycleSnapshot>::failure(
          relations.error());
    s.relations = relations.value();
    count = readSectionCount(in, "OWNERS");
    for (std::size_t i = 0; i < count; ++i) {
      std::string relation, definition;
      in >> std::quoted(relation) >> std::quoted(definition);
      s.relationOwners.emplace(parseId<cad::AssemblyRelationId>(relation),
                               parseId<cad::AssemblyDefinitionId>(definition));
    }
    if (!in)
      throw std::runtime_error("invalid lifecycle data");
    in >> std::ws;
    if (!in.eof())
      throw std::runtime_error("trailing lifecycle data");
    return core::Result<AssemblyLifecycleSnapshot>::success(std::move(s));
  } catch (const std::exception &e) {
    return core::Result<AssemblyLifecycleSnapshot>::failure(
        {core::ErrorCode::io_failure, e.what(), "AssemblyLifecycleSnapshot"});
  }
}
} // namespace duomec::assembly

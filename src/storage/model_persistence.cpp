#include "duomec/storage/model_persistence.hpp"

#include <bit>
#include <cstring>
#include <stdexcept>

namespace duomec::storage {
namespace {
class Writer {
public:
  template <class Integer> void integer(Integer value) {
    using Unsigned = std::make_unsigned_t<Integer>;
    auto converted = static_cast<Unsigned>(value);
    for (std::size_t index = 0; index < sizeof(Integer); ++index)
      bytes_.push_back(static_cast<std::uint8_t>(converted >> (index * 8U)));
  }
  void floating(double value) { integer(std::bit_cast<std::uint64_t>(value)); }
  void string(std::string_view value) {
    integer<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }
  template <class Id> void id(const Id &value) { string(value.value()); }
  template <class Map> void stringMap(const Map &map) {
    integer<std::uint32_t>(static_cast<std::uint32_t>(map.size()));
    for (const auto &[key, value] : map) {
      string(key);
      string(value);
    }
  }
  std::vector<std::uint8_t> take() { return std::move(bytes_); }

private:
  std::vector<std::uint8_t> bytes_;
};
class Reader {
public:
  Reader(std::span<const std::uint8_t> bytes, StorageLimits limits)
      : bytes_(bytes), limits_(limits) {}
  template <class Integer> Integer integer() {
    if (remaining() < sizeof(Integer))
      throw std::runtime_error("truncated manifest field");
    std::uint64_t value{};
    for (std::size_t index = 0; index < sizeof(Integer); ++index)
      value |= static_cast<std::uint64_t>(bytes_[offset_++]) << (index * 8U);
    return static_cast<Integer>(value);
  }
  double floating() { return std::bit_cast<double>(integer<std::uint64_t>()); }
  std::string string() {
    const auto size = integer<std::uint32_t>();
    if (size > limits_.maxMetadataStringSize || size > remaining())
      throw std::runtime_error("invalid manifest string size");
    std::string value(reinterpret_cast<const char *>(bytes_.data() + offset_),
                      size);
    offset_ += size;
    return value;
  }
  template <class Id> Id id() {
    const auto parsed = Id::parse(string());
    if (!parsed)
      throw std::runtime_error("invalid persistent id in manifest");
    return parsed.value();
  }
  std::map<std::string, std::string, std::less<>> stringMap() {
    const auto count = integer<std::uint32_t>();
    if (count > limits_.maxManifestEntries)
      throw std::runtime_error("manifest map exceeds safety limit");
    std::map<std::string, std::string, std::less<>> result;
    for (std::uint32_t index = 0; index < count; ++index)
      if (!result.emplace(string(), string()).second)
        throw std::runtime_error("duplicate manifest map key");
    return result;
  }
  [[nodiscard]] std::size_t remaining() const {
    return bytes_.size() - offset_;
  }

private:
  std::span<const std::uint8_t> bytes_;
  StorageLimits limits_;
  std::size_t offset_{};
};
template <class Values, class Function>
void sequence(Writer &writer, const Values &values, Function function) {
  writer.integer<std::uint32_t>(static_cast<std::uint32_t>(values.size()));
  for (const auto &value : values)
    function(value);
}
void writeReferences(Writer &writer,
                     const assembly::LightweightReferenceMetadata &references) {
  for (const auto &reference : references.references) {
    writer.integer<std::uint8_t>(static_cast<std::uint8_t>(reference.kind));
    writer.id(reference.referenceId);
  }
}
assembly::LightweightReferenceMetadata readReferences(Reader &reader) {
  assembly::LightweightReferenceMetadata result;
  for (auto &reference : result.references) {
    reference.kind = static_cast<assembly::AbsoluteReferenceKind>(
        reader.integer<std::uint8_t>());
    reference.referenceId = reader.id<cad::TopologyReferenceId>();
  }
  return result;
}
void writeRevisionMetadata(Writer &writer,
                           const assembly::RevisionMetadata &metadata) {
  writer.string(metadata.label);
  writer.stringMap(metadata.properties);
}
assembly::RevisionMetadata readRevisionMetadata(Reader &reader) {
  return {reader.string(), reader.stringMap()};
}
std::vector<std::uint8_t>
serializeMetadata(const SourceDocumentMetadata &metadata) {
  Writer writer;
  writer.string("DUOMEC_METADATA");
  writer.integer(metadata.schemaVersion);
  writer.string(metadata.name);
  writer.stringMap(metadata.properties);
  return writer.take();
}
core::Result<SourceDocumentMetadata>
deserializeMetadata(std::span<const std::uint8_t> bytes, StorageLimits limits) {
  try {
    Reader reader(bytes, limits);
    if (reader.string() != "DUOMEC_METADATA")
      throw std::runtime_error("invalid metadata manifest");
    SourceDocumentMetadata result;
    result.schemaVersion = reader.integer<std::uint32_t>();
    if (result.schemaVersion != 1)
      throw std::runtime_error("unsupported metadata schema");
    result.name = reader.string();
    result.properties = reader.stringMap();
    if (reader.remaining() != 0)
      throw std::runtime_error("trailing metadata manifest bytes");
    return core::Result<SourceDocumentMetadata>::success(std::move(result));
  } catch (const std::exception &exception) {
    return core::Result<SourceDocumentMetadata>::failure(
        {core::ErrorCode::io_failure, exception.what(), "DocumentMetadata"});
  }
}
ChunkWriteRequest chunk(std::string id, ChunkType type,
                        std::vector<std::uint8_t> payload) {
  return {std::move(id),
          type,
          1,
          ChunkCodec::None,
          ChunkCriticality::Required,
          {},
          std::move(payload)};
}
core::Result<bool>
addOpaque(DuomecContainerWriter &writer,
          std::span<const OpaqueAuthoritativePayload> payloads) {
  for (const auto &payload : payloads) {
    auto added = writer.addChunk(
        {payload.chunkId, payload.type, payload.schemaVersion, ChunkCodec::None,
         ChunkCriticality::Optional, payload.contentHash, payload.bytes});
    if (!added)
      return added;
  }
  return core::Result<bool>::success(true);
}
} // namespace

std::vector<std::uint8_t>
serializeRegistry(const assembly::DefinitionRegistry &registry) {
  Writer writer;
  writer.string("DUOMEC_REGISTRY");
  writer.integer<std::uint32_t>(1);
  sequence(writer, registry.partDefinitions(), [&](const auto &definition) {
    writer.id(definition->id);
    writer.string(definition->name);
    writer.integer<std::uint8_t>(
        static_cast<std::uint8_t>(definition->storage));
    writer.integer<std::uint8_t>(definition->defaultRevision.has_value());
    if (definition->defaultRevision)
      writer.id(*definition->defaultRevision);
    writeReferences(writer, definition->absoluteReferences);
    writer.stringMap(definition->metadata);
  });
  sequence(writer, registry.assemblyDefinitions(), [&](const auto &definition) {
    writer.id(definition->id);
    writer.string(definition->name);
    writer.integer<std::uint8_t>(
        static_cast<std::uint8_t>(definition->storage));
    writer.integer<std::uint8_t>(definition->defaultRevision.has_value());
    if (definition->defaultRevision)
      writer.id(*definition->defaultRevision);
    writeReferences(writer, definition->absoluteReferences);
    writer.stringMap(definition->metadata);
  });
  sequence(writer, registry.partRevisions(), [&](const auto &revision) {
    writer.id(revision->id);
    writer.id(revision->definitionId);
    writer.string(revision->authoringHash.value());
    writer.string(revision->geometryHash.value());
    writer.string(revision->displayRelevantHash.value());
    writeRevisionMetadata(writer, revision->metadata);
    sequence(writer, revision->bodies, [&](const auto &body) {
      writer.id(body.id);
      writer.string(body.geometryHash.value());
      writer.stringMap(body.metadata);
    });
  });
  sequence(writer, registry.assemblyRevisions(), [&](const auto &revision) {
    writer.id(revision->id);
    writer.id(revision->definitionId);
    writer.string(revision->authoringHash.value());
    writeRevisionMetadata(writer, revision->metadata);
  });
  return writer.take();
}
core::Result<std::shared_ptr<assembly::DefinitionRegistry>>
deserializeRegistry(std::span<const std::uint8_t> bytes, StorageLimits limits) {
  try {
    Reader reader(bytes, limits);
    if (reader.string() != "DUOMEC_REGISTRY" ||
        reader.integer<std::uint32_t>() != 1)
      throw std::runtime_error("unsupported registry manifest");
    auto registry = std::make_shared<assembly::DefinitionRegistry>();
    const auto partCount = reader.integer<std::uint32_t>();
    if (partCount > limits.maxManifestEntries)
      throw std::runtime_error("part definition count exceeds safety limit");
    for (std::uint32_t index = 0; index < partCount; ++index) {
      assembly::PartDefinition definition;
      definition.id = reader.id<cad::PartDefinitionId>();
      definition.name = reader.string();
      definition.storage = static_cast<assembly::DefinitionStorageClass>(
          reader.integer<std::uint8_t>());
      if (reader.integer<std::uint8_t>())
        definition.defaultRevision = reader.id<cad::PartRevisionId>();
      definition.absoluteReferences = readReferences(reader);
      definition.metadata = reader.stringMap();
      if (!registry->addPartDefinition(std::move(definition)))
        throw std::runtime_error("duplicate part definition");
    }
    const auto assemblyCount = reader.integer<std::uint32_t>();
    if (assemblyCount > limits.maxManifestEntries)
      throw std::runtime_error(
          "assembly definition count exceeds safety limit");
    for (std::uint32_t index = 0; index < assemblyCount; ++index) {
      assembly::AssemblyDefinition definition;
      definition.id = reader.id<cad::AssemblyDefinitionId>();
      definition.name = reader.string();
      definition.storage = static_cast<assembly::DefinitionStorageClass>(
          reader.integer<std::uint8_t>());
      if (reader.integer<std::uint8_t>())
        definition.defaultRevision = reader.id<cad::AssemblyRevisionId>();
      definition.absoluteReferences = readReferences(reader);
      definition.metadata = reader.stringMap();
      if (!registry->addAssemblyDefinition(std::move(definition)))
        throw std::runtime_error("duplicate assembly definition");
    }
    const auto partRevisionCount = reader.integer<std::uint32_t>();
    if (partRevisionCount > limits.maxManifestEntries)
      throw std::runtime_error("part revision count exceeds safety limit");
    for (std::uint32_t index = 0; index < partRevisionCount; ++index) {
      assembly::PartRevision revision;
      revision.id = reader.id<cad::PartRevisionId>();
      revision.definitionId = reader.id<cad::PartDefinitionId>();
      revision.authoringHash = assembly::AuthoringHash(reader.string());
      revision.geometryHash = assembly::GeometryHash(reader.string());
      revision.displayRelevantHash =
          assembly::DisplayRelevantHash(reader.string());
      revision.metadata = readRevisionMetadata(reader);
      const auto bodyCount = reader.integer<std::uint32_t>();
      if (bodyCount > limits.maxManifestEntries)
        throw std::runtime_error("body revision count exceeds safety limit");
      revision.bodies.reserve(bodyCount);
      for (std::uint32_t bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex) {
        assembly::BodyRevision body;
        body.id = reader.id<cad::BodyRevisionId>();
        body.geometryHash = assembly::GeometryHash(reader.string());
        body.metadata = reader.stringMap();
        revision.bodies.push_back(std::move(body));
      }
      if (!registry->commitPartRevision(std::move(revision)))
        throw std::runtime_error("invalid part revision");
    }
    const auto assemblyRevisionCount = reader.integer<std::uint32_t>();
    if (assemblyRevisionCount > limits.maxManifestEntries)
      throw std::runtime_error("assembly revision count exceeds safety limit");
    for (std::uint32_t index = 0; index < assemblyRevisionCount; ++index) {
      assembly::AssemblyRevision revision;
      revision.id = reader.id<cad::AssemblyRevisionId>();
      revision.definitionId = reader.id<cad::AssemblyDefinitionId>();
      revision.authoringHash = assembly::AuthoringHash(reader.string());
      revision.metadata = readRevisionMetadata(reader);
      if (!registry->commitAssemblyRevision(std::move(revision)))
        throw std::runtime_error("invalid assembly revision");
    }
    if (reader.remaining() != 0)
      throw std::runtime_error("trailing registry manifest bytes");
    return core::Result<std::shared_ptr<assembly::DefinitionRegistry>>::success(
        std::move(registry));
  } catch (const std::exception &exception) {
    return core::Result<std::shared_ptr<assembly::DefinitionRegistry>>::failure(
        {core::ErrorCode::io_failure, exception.what(), "DefinitionRegistry"});
  }
}

std::vector<std::uint8_t>
serializeOccurrenceGraph(const assembly::AssemblyRuntimeGraph &graph) {
  Writer writer;
  writer.string("DUOMEC_OCCURRENCES");
  writer.integer<std::uint32_t>(1);
  sequence(writer, graph.occurrences(), [&](const auto &occurrence) {
    writer.id(occurrence.id);
    std::visit(
        [&](const auto &reference) {
          using Reference = std::decay_t<decltype(reference)>;
          if constexpr (std::is_same_v<Reference,
                                       assembly::PartDefinitionRef>) {
            writer.integer<std::uint8_t>(0);
            writer.id(reference.definitionId);
            writer.id(reference.revisionId);
            writer.string(reference.authoringHash.value());
            writer.string(reference.geometryHash.value());
            writer.string(reference.displayRelevantHash.value());
          } else {
            writer.integer<std::uint8_t>(1);
            writer.id(reference.definitionId);
            writer.id(reference.revisionId);
            writer.string(reference.authoringHash.value());
          }
        },
        occurrence.reference);
    writer.integer<std::uint8_t>(occurrence.parent.has_value());
    if (occurrence.parent)
      writer.id(*occurrence.parent);
    for (const auto value : occurrence.localTransform.matrix)
      writer.floating(value);
    writer.integer<std::uint8_t>(occurrence.visible);
    writer.integer<std::uint8_t>(occurrence.suppressed);
    writer.stringMap(occurrence.appearanceOverrides);
    writer.stringMap(occurrence.metadata);
    writer.integer<std::uint8_t>(
        static_cast<std::uint8_t>(occurrence.mobility));
    writer.integer<std::uint8_t>(
        static_cast<std::uint8_t>(occurrence.subassemblyMode));
  });
  return writer.take();
}
core::Result<std::unique_ptr<assembly::AssemblyRuntimeGraph>>
deserializeOccurrenceGraph(
    std::span<const std::uint8_t> bytes,
    std::shared_ptr<const assembly::DefinitionRegistry> registry,
    StorageLimits limits) {
  try {
    Reader reader(bytes, limits);
    if (reader.string() != "DUOMEC_OCCURRENCES" ||
        reader.integer<std::uint32_t>() != 1)
      throw std::runtime_error("unsupported occurrence manifest");
    const auto count = reader.integer<std::uint32_t>();
    if (count > limits.maxOccurrenceRecords)
      throw std::runtime_error("occurrence count exceeds safety limit");
    std::vector<assembly::AssemblyOccurrence> pending;
    pending.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
      assembly::AssemblyOccurrence occurrence;
      occurrence.id = reader.id<cad::OccurrenceId>();
      const auto kind = reader.integer<std::uint8_t>();
      if (kind == 0) {
        assembly::PartDefinitionRef reference;
        reference.definitionId = reader.id<cad::PartDefinitionId>();
        reference.revisionId = reader.id<cad::PartRevisionId>();
        reference.authoringHash = assembly::AuthoringHash(reader.string());
        reference.geometryHash = assembly::GeometryHash(reader.string());
        reference.displayRelevantHash =
            assembly::DisplayRelevantHash(reader.string());
        occurrence.reference = std::move(reference);
      } else if (kind == 1) {
        assembly::AssemblyDefinitionRef reference;
        reference.definitionId = reader.id<cad::AssemblyDefinitionId>();
        reference.revisionId = reader.id<cad::AssemblyRevisionId>();
        reference.authoringHash = assembly::AuthoringHash(reader.string());
        occurrence.reference = std::move(reference);
      } else {
        throw std::runtime_error("invalid occurrence reference kind");
      }
      if (reader.integer<std::uint8_t>())
        occurrence.parent = reader.id<cad::OccurrenceId>();
      for (auto &value : occurrence.localTransform.matrix)
        value = reader.floating();
      occurrence.visible = reader.integer<std::uint8_t>() != 0;
      occurrence.suppressed = reader.integer<std::uint8_t>() != 0;
      occurrence.appearanceOverrides = reader.stringMap();
      occurrence.metadata = reader.stringMap();
      occurrence.mobility = static_cast<assembly::PlacementMobility>(
          reader.integer<std::uint8_t>());
      occurrence.subassemblyMode = static_cast<assembly::SubassemblySolveMode>(
          reader.integer<std::uint8_t>());
      pending.push_back(std::move(occurrence));
    }
    if (reader.remaining() != 0)
      throw std::runtime_error("trailing occurrence manifest bytes");
    auto graph = std::make_unique<assembly::AssemblyRuntimeGraph>(registry);
    while (!pending.empty()) {
      bool progressed = false;
      for (std::size_t index = 0; index < pending.size();) {
        if (!pending[index].parent || graph->find(*pending[index].parent)) {
          const auto added = graph->addOccurrence(std::move(pending[index]));
          if (!added)
            throw std::runtime_error(added.error().message);
          pending[index] = std::move(pending.back());
          pending.pop_back();
          progressed = true;
        } else {
          ++index;
        }
      }
      if (!progressed)
        throw std::runtime_error("invalid or cyclic occurrence hierarchy");
    }
    return core::Result<std::unique_ptr<assembly::AssemblyRuntimeGraph>>::
        success(std::move(graph));
  } catch (const std::exception &exception) {
    return core::Result<std::unique_ptr<assembly::AssemblyRuntimeGraph>>::
        failure({core::ErrorCode::io_failure, exception.what(),
                 "AssemblyRuntimeGraph"});
  }
}

core::Result<bool>
savePartContainer(const std::filesystem::path &path,
                  const SourceDocumentMetadata &metadata,
                  const assembly::DefinitionRegistry &registry,
                  std::span<const OpaqueAuthoritativePayload> opaquePayloads,
                  bool simulateFailureBeforeReplace) {
  DuomecContainerWriter writer(ContainerType::Part);
  auto result =
      writer.addChunk(chunk("document-metadata", ChunkType::DocumentMetadata,
                            serializeMetadata(metadata)));
  if (result)
    result = writer.addChunk(chunk("definition-registry",
                                   ChunkType::DefinitionRegistry,
                                   serializeRegistry(registry)));
  if (result)
    result = addOpaque(writer, opaquePayloads);
  if (!result)
    return result;
  return writer.writeAtomic(path, simulateFailureBeforeReplace);
}
core::Result<bool> saveAssemblyContainer(
    const std::filesystem::path &path, const SourceDocumentMetadata &metadata,
    const assembly::DefinitionRegistry &registry,
    const assembly::AssemblyRuntimeGraph &graph,
    std::span<const OpaqueAuthoritativePayload> opaquePayloads,
    bool simulateFailureBeforeReplace) {
  DuomecContainerWriter writer(ContainerType::Assembly);
  auto result =
      writer.addChunk(chunk("document-metadata", ChunkType::DocumentMetadata,
                            serializeMetadata(metadata)));
  if (result)
    result = writer.addChunk(chunk("definition-registry",
                                   ChunkType::DefinitionRegistry,
                                   serializeRegistry(registry)));
  if (result)
    result = writer.addChunk(chunk("occurrence-graph",
                                   ChunkType::AssemblyOccurrenceGraph,
                                   serializeOccurrenceGraph(graph)));
  if (result) {
    const auto relations = assembly::serialize(graph.relations());
    result = writer.addChunk(chunk("assembly-relations",
                                   ChunkType::AssemblyRelations,
                                   {relations.begin(), relations.end()}));
  }
  if (result)
    result = addOpaque(writer, opaquePayloads);
  if (!result)
    return result;
  return writer.writeAtomic(path, simulateFailureBeforeReplace);
}
core::Result<SourceDocumentMetadata>
readDocumentMetadata(const DuomecContainerReader &reader,
                     StorageLimits limits) {
  const auto bytes = reader.readChunk(ChunkType::DocumentMetadata);
  if (!bytes)
    return core::Result<SourceDocumentMetadata>::failure(bytes.error());
  return deserializeMetadata(bytes.value(), limits);
}
core::Result<std::shared_ptr<assembly::DefinitionRegistry>>
readDefinitionRegistry(const DuomecContainerReader &reader,
                       StorageLimits limits) {
  const auto bytes = reader.readChunk(ChunkType::DefinitionRegistry);
  if (!bytes)
    return core::Result<std::shared_ptr<assembly::DefinitionRegistry>>::failure(
        bytes.error());
  return deserializeRegistry(bytes.value(), limits);
}
core::Result<LoadedAssembly>
loadAssemblyContainer(const std::filesystem::path &path, StorageLimits limits) {
  const auto readerResult = DuomecContainerReader::open(path, limits);
  if (!readerResult)
    return core::Result<LoadedAssembly>::failure(readerResult.error());
  const auto &reader = readerResult.value();
  if (reader.header().type != ContainerType::Assembly)
    return core::Result<LoadedAssembly>::failure(
        {core::ErrorCode::invalid_argument, "not an assembly container",
         path.string()});
  auto metadata = readDocumentMetadata(reader, limits);
  auto registry = readDefinitionRegistry(reader, limits);
  if (!metadata)
    return core::Result<LoadedAssembly>::failure(metadata.error());
  if (!registry)
    return core::Result<LoadedAssembly>::failure(registry.error());
  const auto graphBytes = reader.readChunk(ChunkType::AssemblyOccurrenceGraph);
  if (!graphBytes)
    return core::Result<LoadedAssembly>::failure(graphBytes.error());
  auto graph =
      deserializeOccurrenceGraph(graphBytes.value(), registry.value(), limits);
  if (!graph)
    return core::Result<LoadedAssembly>::failure(graph.error());
  const auto relationBytes = reader.readChunk(ChunkType::AssemblyRelations);
  if (!relationBytes)
    return core::Result<LoadedAssembly>::failure(relationBytes.error());
  const std::string relationText(relationBytes.value().begin(),
                                 relationBytes.value().end());
  auto relations = assembly::deserialize(relationText);
  if (!relations)
    return core::Result<LoadedAssembly>::failure(relations.error());
  graph.value()->relations() = std::move(relations.value());
  auto loadedMetadata = std::move(metadata).value();
  auto loadedRegistry = std::move(registry).value();
  auto loadedGraph = std::move(graph).value();
  return core::Result<LoadedAssembly>::success({std::move(loadedMetadata),
                                                std::move(loadedRegistry),
                                                std::move(loadedGraph)});
}

} // namespace duomec::storage

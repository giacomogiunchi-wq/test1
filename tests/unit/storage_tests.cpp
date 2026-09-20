#include "duomec/storage/local_cache.hpp"
#include "duomec/storage/model_persistence.hpp"
#include "duomec/storage/sqlite_cache_catalog.hpp"

#include <cassert>
#include <fstream>
#include <thread>

namespace {
using namespace duomec;
using namespace duomec::assembly;
using namespace duomec::storage;

struct Fixture {
  std::shared_ptr<DefinitionRegistry> registry =
      std::make_shared<DefinitionRegistry>();
  PartDefinition part;
  PartRevision revision;
  PartDefinitionRef reference;
  Fixture() {
    part.name = "Bolt";
    part.storage = DefinitionStorageClass::EmbeddedVirtual;
    part.metadata["number"] = "B-100";
    revision.definitionId = part.id;
    revision.authoringHash = AuthoringHash("authoring-a");
    revision.geometryHash = GeometryHash("geometry-a");
    revision.displayRelevantHash = DisplayRelevantHash("display-a");
    revision.metadata.label = "Released";
    BodyRevision body;
    body.geometryHash = GeometryHash("body-geometry-a");
    body.metadata["name"] = "Body";
    revision.bodies.push_back(body);
    part.defaultRevision = revision.id;
    assert(registry->addPartDefinition(part));
    assert(registry->commitPartRevision(revision));
    reference = registry->reference(part.id, revision.id).value();
  }
};
std::filesystem::path temporary(std::string_view name) {
  return std::filesystem::temp_directory_path() /
         (std::string("duomec-") + std::string(name));
}
void overwriteByte(const std::filesystem::path &path, std::uint64_t offset,
                   std::uint8_t value) {
  std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
  file.seekp(static_cast<std::streamoff>(offset));
  file.put(static_cast<char>(value));
}
void put64(const std::filesystem::path &path, std::uint64_t offset,
           std::uint64_t value) {
  std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
  file.seekp(static_cast<std::streamoff>(offset));
  for (unsigned index = 0; index < 8; ++index)
    file.put(static_cast<char>(value >> (index * 8U)));
}
} // namespace

int main() {
  using namespace duomec;
  using namespace duomec::assembly;
  using namespace duomec::storage;
  Fixture fixture;
  SourceDocumentMetadata metadata;
  metadata.name = "Storage test";
  metadata.properties["project"] = "M5.1C";
  const auto partPath = temporary("part.duomecpart");
  std::filesystem::remove(partPath);
  const std::vector<std::uint8_t> authoring(1024 * 1024, 0x5a);
  OpaqueAuthoritativePayload opaque{"ocaf-authoring",
                                    ChunkType::AuthoringPayload, 1,
                                    "authoring-a", authoring};
  assert(savePartContainer(partPath, metadata, *fixture.registry,
                           std::span(&opaque, 1)));
  auto partReader = DuomecContainerReader::open(partPath);
  assert(partReader && partReader.value().header().type == ContainerType::Part);
  const auto bytesAfterOpen = partReader.value().bytesRead();
  assert(bytesAfterOpen < authoring.size());
  assert(readDocumentMetadata(partReader.value()).value().name ==
         metadata.name);
  auto loadedRegistry = readDefinitionRegistry(partReader.value());
  assert(loadedRegistry);
  const auto loadedRevision =
      loadedRegistry.value()->partRevision(fixture.revision.id);
  assert(loadedRevision && loadedRevision->bodies.size() == 1);
  assert(loadedRevision->geometryHash == fixture.revision.geometryHash);
  assert(loadedRegistry.value()->partDefinition(fixture.part.id)->storage ==
         DefinitionStorageClass::EmbeddedVirtual);
  assert(partReader.value().bytesRead() < partReader.value().fileSize());
  assert(partReader.value().readChunk("ocaf-authoring").value() == authoring);

  AssemblyRuntimeGraph graph(fixture.registry);
  std::vector<cad::OccurrenceId> ids;
  ids.reserve(10'000);
  for (std::size_t index = 0; index < 10'000; ++index) {
    AssemblyOccurrence occurrence;
    occurrence.reference = fixture.reference;
    occurrence.localTransform.matrix[12] = static_cast<double>(index);
    occurrence.visible = index % 2 == 0;
    occurrence.suppressed = index == 9;
    occurrence.appearanceOverrides["color"] = index == 1 ? "blue" : "";
    occurrence.metadata["index"] = std::to_string(index);
    occurrence.mobility = PlacementMobility::Floating;
    occurrence.subassemblyMode = SubassemblySolveMode::Rigid;
    if (index == 1)
      occurrence.parent = ids.front();
    ids.push_back(graph.addOccurrence(std::move(occurrence)).value());
  }
  AssemblyRelation relation;
  relation.type = RelationType::Coincident;
  RelationEndpoint endpoint;
  endpoint.occurrenceId = ids.front();
  relation.endpoints.push_back(endpoint);
  graph.relations().relations.push_back(relation);
  const auto assemblyPath = temporary("assembly.duomecasm");
  std::filesystem::remove(assemblyPath);
  assert(saveAssemblyContainer(assemblyPath, metadata, *fixture.registry, graph,
                               std::span(&opaque, 1)));
  auto assemblyReader = DuomecContainerReader::open(assemblyPath);
  assert(assemblyReader && assemblyReader.value().listChunks().size() == 5);
  assert(assemblyReader.value().bytesRead() <
         assemblyReader.value().fileSize());
  auto loadedAssembly = loadAssemblyContainer(assemblyPath);
  assert(loadedAssembly && loadedAssembly.value().graph->size() == 10'000);
  assert(loadedAssembly.value().graph->find(ids.front())->id == ids.front());
  assert(loadedAssembly.value().graph->find(ids[1])->parent == ids.front());
  assert(loadedAssembly.value().graph->find(ids[9])->suppressed);
  assert(loadedAssembly.value().graph->relations().relations.size() == 1);
  const auto firstRef = std::get<PartDefinitionRef>(
      loadedAssembly.value().graph->find(ids.front())->reference);
  const auto lastRef = std::get<PartDefinitionRef>(
      loadedAssembly.value().graph->find(ids.back())->reference);
  assert(firstRef == lastRef);
  assert(loadedAssembly.value().registry->partRevisions().size() == 1);
  assert(loadedAssembly.value().registry->partRevision(firstRef.revisionId) ==
         loadedAssembly.value().registry->partRevision(lastRef.revisionId));

  // Unknown optional chunks are directory entries that can be skipped; unknown
  // required chunks fail before any payload is trusted.
  const auto optionalPath = temporary("optional.duomecpart");
  DuomecContainerWriter optionalWriter(ContainerType::Part);
  assert(optionalWriter.addChunk({"future",
                                  static_cast<ChunkType>(999),
                                  1,
                                  static_cast<ChunkCodec>(77),
                                  ChunkCriticality::Optional,
                                  {},
                                  {1, 2, 3}}));
  assert(!optionalWriter.addChunk({"future",
                                   ChunkType::DocumentMetadata,
                                   1,
                                   ChunkCodec::None,
                                   ChunkCriticality::Optional,
                                   {},
                                   {4}}));
  assert(optionalWriter.writeAtomic(optionalPath));
  assert(DuomecContainerReader::open(optionalPath));
#ifdef DUOMEC_ENABLE_ZSTD
  const auto zstdPath = temporary("zstd.duomecpart");
  DuomecContainerWriter zstdWriter(ContainerType::Part);
  assert(zstdWriter.addChunk({"compressed",
                              ChunkType::AuthoringPayload,
                              1,
                              ChunkCodec::Zstd,
                              ChunkCriticality::Optional,
                              {},
                              authoring}));
  assert(zstdWriter.writeAtomic(zstdPath));
  const auto zstdReader = DuomecContainerReader::open(zstdPath);
  assert(zstdReader &&
         zstdReader.value().readChunk("compressed").value() == authoring);
  assert(zstdReader.value().findChunk("compressed")->storedSize <
         authoring.size());
  std::filesystem::remove(zstdPath);
#endif
  const auto requiredPath = temporary("required.duomecpart");
  DuomecContainerWriter requiredWriter(ContainerType::Part);
  assert(requiredWriter.addChunk({"future",
                                  static_cast<ChunkType>(999),
                                  1,
                                  ChunkCodec::None,
                                  ChunkCriticality::Required,
                                  {},
                                  {1}}));
  // Writer validation deliberately refuses to publish a file readers reject.
  assert(!requiredWriter.writeAtomic(requiredPath));

  // Atomic staged failure leaves the previously valid authoritative file.
  const auto originalSize = std::filesystem::file_size(partPath);
  assert(!savePartContainer(partPath, {1, "replacement", {}}, *fixture.registry,
                            {}, true));
  assert(std::filesystem::file_size(partPath) == originalSize);
  assert(readDocumentMetadata(DuomecContainerReader::open(partPath).value())
             .value()
             .name == metadata.name);

  // Corruption and bounds checks.
  const auto corruptPath = temporary("corrupt.duomecpart");
  std::filesystem::copy_file(partPath, corruptPath,
                             std::filesystem::copy_options::overwrite_existing);
  auto corruptReader = DuomecContainerReader::open(corruptPath).value();
  const auto payloadOffset =
      corruptReader.findChunk(ChunkType::DocumentMetadata)->payloadOffset;
  overwriteByte(corruptPath, payloadOffset, 0xff);
  assert(!DuomecContainerReader::open(corruptPath)
              .value()
              .readChunk(ChunkType::DocumentMetadata));
  const auto truncatedPath = temporary("truncated.duomecpart");
  std::filesystem::copy_file(partPath, truncatedPath,
                             std::filesystem::copy_options::overwrite_existing);
  std::filesystem::resize_file(truncatedPath,
                               std::filesystem::file_size(truncatedPath) - 10);
  assert(!DuomecContainerReader::open(truncatedPath));
  const auto invalidOffsetPath = temporary("offset.duomecpart");
  std::filesystem::copy_file(partPath, invalidOffsetPath,
                             std::filesystem::copy_options::overwrite_existing);
  const auto toc =
      DuomecContainerReader::open(invalidOffsetPath).value().header().tocOffset;
  put64(invalidOffsetPath, toc + 48, UINT64_MAX);
  assert(!DuomecContainerReader::open(invalidOffsetPath));
  const auto invalidSizePath = temporary("size.duomecpart");
  std::filesystem::copy_file(partPath, invalidSizePath,
                             std::filesystem::copy_options::overwrite_existing);
  put64(invalidSizePath, toc + 56, UINT64_MAX);
  assert(!DuomecContainerReader::open(invalidSizePath));
  const auto invalidTocPath = temporary("toc.duomecpart");
  std::filesystem::copy_file(partPath, invalidTocPath,
                             std::filesystem::copy_options::overwrite_existing);
  put64(invalidTocPath, 20, UINT64_MAX);
  assert(!DuomecContainerReader::open(invalidTocPath));
  const auto invalidMagicPath = temporary("magic.duomecpart");
  std::filesystem::copy_file(partPath, invalidMagicPath,
                             std::filesystem::copy_options::overwrite_existing);
  overwriteByte(invalidMagicPath, 0, 0);
  assert(!DuomecContainerReader::open(invalidMagicPath));
  const auto versionPath = temporary("version.duomecpart");
  std::filesystem::copy_file(partPath, versionPath,
                             std::filesystem::copy_options::overwrite_existing);
  overwriteByte(versionPath, 8, 2);
  assert(!DuomecContainerReader::open(versionPath));

  // Disposable content-addressed cache.
  const auto cacheRoot = temporary("cache");
  std::filesystem::remove_all(cacheRoot);
  auto catalog = std::make_shared<InMemoryCacheCatalog>();
  LocalAssetCache cache(cacheRoot, catalog);
  const auto key = geometryCacheKey(AssetKind::Mesh,
                                    fixture.revision.geometryHash, 1, "gen-1");
  const std::vector<std::uint8_t> asset{1, 2, 3, 4, 5};
  assert(!cache.get(key).value());
  assert(cache.put(key, asset).value());
  assert(!cache.put(key, asset).value());
  assert(cache.get(key).value().value() == asset);
  assert(cache.validate(key));
  assert(cache.pathFor(key) == cache.pathFor(key));
  const auto differentGenerator = geometryCacheKey(
      AssetKind::Mesh, fixture.revision.geometryHash, 1, "gen-2");
  assert(!cache.get(differentGenerator).value());
  const auto differentFormat = geometryCacheKey(
      AssetKind::Mesh, fixture.revision.geometryHash, 2, "gen-1");
  assert(!cache.get(differentFormat).value());
  const auto differentGeometry =
      geometryCacheKey(AssetKind::Mesh, GeometryHash("geometry-b"), 1, "gen-1");
  assert(key != differentGeometry);
  const auto transformEvent = graph.updateOccurrenceTransform(ids.front(), {});
  assert(transformEvent &&
         !transformEvent.value().contains(InvalidationDomain::ExactGeometry));
  assert(geometryCacheKey(AssetKind::Mesh, fixture.revision.geometryHash, 1,
                          "gen-1") == key);
  std::vector<std::thread> writers;
  for (int index = 0; index < 8; ++index)
    writers.emplace_back([&] { assert(cache.put(key, asset)); });
  for (auto &writer : writers)
    writer.join();
  assert(catalog->size() == 1);
  overwriteByte(cache.pathFor(key),
                std::filesystem::file_size(cache.pathFor(key)) - 1, 0xff);
  assert(!cache.get(key).value());
  assert(!std::filesystem::exists(cache.pathFor(key)));
  assert(cache.put(key, asset));
  assert(cache.remove(key).value());
  assert(!cache.get(key).value());
  assert(cache.put(key, asset));
  assert(cache.clear());
  assert(catalog->size() == 0 && !std::filesystem::exists(cacheRoot));

#ifdef DUOMEC_ENABLE_SQLITE_CACHE
  const auto sqlitePath = temporary("cache.sqlite");
  std::filesystem::remove(sqlitePath);
  {
    auto sqliteCatalog = std::make_shared<SqliteCacheCatalog>(sqlitePath);
    LocalAssetCache sqliteCache(cacheRoot, sqliteCatalog);
    assert(sqliteCache.put(key, asset));
    assert(sqliteCatalog->size() == 1);
    assert(sqliteCatalog->find(key));
    assert(sqliteCache.get(key).value().value() == asset);
    assert(sqliteCache.clear());
  }
  std::filesystem::remove(sqlitePath);
  std::filesystem::remove(sqlitePath.string() + "-wal");
  std::filesystem::remove(sqlitePath.string() + "-shm");
#endif

  for (const auto &path :
       {partPath, assemblyPath, optionalPath, requiredPath, corruptPath,
        truncatedPath, invalidOffsetPath, invalidSizePath, invalidTocPath,
        invalidMagicPath, versionPath})
    std::filesystem::remove(path);
}

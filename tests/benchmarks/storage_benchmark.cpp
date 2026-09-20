#include "duomec/storage/local_cache.hpp"
#include "duomec/storage/model_persistence.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

namespace {
using Clock = std::chrono::steady_clock;
template <class Function> std::uint64_t measure(Function &&function) {
  const auto start = Clock::now();
  function();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() -
                                                            start)
          .count());
}
void report(std::string_view operation, std::vector<std::uint64_t> samples) {
  std::sort(samples.begin(), samples.end());
  const auto percentile = [&](double p) {
    return samples[static_cast<std::size_t>(
        p * static_cast<double>(samples.size() - 1))];
  };
  std::cout << operation << ',' << percentile(0.5) << ',' << percentile(0.95)
            << ',' << percentile(0.99) << ',' << samples.back() << '\n';
}
} // namespace

int main() {
  using namespace duomec::assembly;
  using namespace duomec::storage;
  auto registry = std::make_shared<DefinitionRegistry>();
  PartDefinition part;
  PartRevision revision;
  revision.definitionId = part.id;
  revision.authoringHash = AuthoringHash("benchmark-authoring");
  revision.geometryHash = GeometryHash("benchmark-geometry");
  revision.displayRelevantHash = DisplayRelevantHash("benchmark-display");
  part.defaultRevision = revision.id;
  if (!registry->addPartDefinition(part) ||
      !registry->commitPartRevision(revision))
    return 1;
  const auto reference = registry->reference(part.id, revision.id).value();
  AssemblyRuntimeGraph graph(registry);
  for (std::size_t index = 0; index < 10'000; ++index) {
    AssemblyOccurrence occurrence;
    occurrence.reference = reference;
    occurrence.localTransform.matrix[12] = static_cast<double>(index);
    if (!graph.addOccurrence(std::move(occurrence)))
      return 2;
  }
  const std::vector<std::uint8_t> heavyPayload(8 * 1024 * 1024, 0x5a);
  OpaqueAuthoritativePayload opaque{"opaque-authoring",
                                    ChunkType::AuthoringPayload, 1,
                                    "benchmark-authoring", heavyPayload};
  const auto path = std::filesystem::temp_directory_path() /
                    "duomec-m5-1c-benchmark.duomecasm";
  const auto cachePath =
      std::filesystem::temp_directory_path() / "duomec-m5-1c-benchmark-cache";
  std::filesystem::remove(path);
  std::filesystem::remove_all(cachePath);
  std::vector<std::uint64_t> save, open, manifests, random;
  for (int sample = 0; sample < 7; ++sample) {
    save.push_back(measure([&] {
      if (!saveAssemblyContainer(path, {1, "benchmark", {}}, *registry, graph,
                                 std::span(&opaque, 1)))
        std::terminate();
    }));
    open.push_back(measure([&] {
      if (!DuomecContainerReader::open(path))
        std::terminate();
    }));
    manifests.push_back(measure([&] {
      auto reader = DuomecContainerReader::open(path).value();
      if (!readDocumentMetadata(reader) || !readDefinitionRegistry(reader) ||
          !reader.readChunk(ChunkType::AssemblyOccurrenceGraph))
        std::terminate();
    }));
    random.push_back(measure([&] {
      auto reader = DuomecContainerReader::open(path).value();
      if (!reader.readChunk(ChunkType::AssemblyRelations))
        std::terminate();
    }));
  }
  auto reader = DuomecContainerReader::open(path).value();
  const auto metadataBytes = reader.bytesRead();
  if (!readDocumentMetadata(reader) || !readDefinitionRegistry(reader) ||
      !reader.readChunk(ChunkType::AssemblyOccurrenceGraph))
    return 3;
  const auto manifestBytes = reader.bytesRead();
  auto catalog = std::make_shared<InMemoryCacheCatalog>();
  LocalAssetCache cache(cachePath, catalog);
  const auto key = geometryCacheKey(AssetKind::Mesh, revision.geometryHash, 1,
                                    "benchmark-generator");
  const std::vector<std::uint8_t> asset(256 * 1024, 0x33);
  std::vector<std::uint64_t> coldPut, warmGet, sharedLookup;
  coldPut.push_back(measure([&] {
    if (!cache.put(key, asset))
      std::terminate();
  }));
  for (int sample = 0; sample < 100; ++sample) {
    warmGet.push_back(measure([&] {
      if (!cache.get(key).value())
        std::terminate();
    }));
    sharedLookup.push_back(measure([&] {
      const auto shared = geometryCacheKey(
          AssetKind::Mesh, revision.geometryHash, 1, "benchmark-generator");
      if (shared != key)
        std::terminate();
    }));
  }
  std::cout << "operation,median_us,p95_us,p99_us,max_us\n";
  report("save_10000", save);
  report("open_header_toc", open);
  report("load_manifests", manifests);
  report("random_chunk", random);
  report("cold_cache_put", coldPut);
  report("warm_cache_get", warmGet);
  report("shared_revision_key", sharedLookup);
  std::cout << "authoritative_file_bytes," << reader.fileSize() << "\n"
            << "header_toc_bytes," << metadataBytes << "\n"
            << "metadata_manifest_bytes," << manifestBytes << "\n";
  std::filesystem::remove(path);
  cache.clear();
}

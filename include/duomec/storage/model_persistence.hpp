#pragma once

#include "duomec/assembly/runtime_model.hpp"
#include "duomec/storage/container.hpp"

namespace duomec::storage {

struct SourceDocumentMetadata {
  std::uint32_t schemaVersion{1};
  std::string name;
  std::map<std::string, std::string, std::less<>> properties;
};

struct OpaqueAuthoritativePayload {
  std::string chunkId;
  ChunkType type{ChunkType::AuthoringPayload};
  std::uint32_t schemaVersion{1};
  std::string contentHash;
  std::vector<std::uint8_t> bytes;
};

struct LoadedAssembly {
  SourceDocumentMetadata metadata;
  std::shared_ptr<assembly::DefinitionRegistry> registry;
  std::unique_ptr<assembly::AssemblyRuntimeGraph> graph;
};

[[nodiscard]] std::vector<std::uint8_t>
serializeRegistry(const assembly::DefinitionRegistry &registry);
[[nodiscard]] core::Result<std::shared_ptr<assembly::DefinitionRegistry>>
deserializeRegistry(std::span<const std::uint8_t> bytes,
                    StorageLimits limits = {});
[[nodiscard]] std::vector<std::uint8_t>
serializeOccurrenceGraph(const assembly::AssemblyRuntimeGraph &graph);
[[nodiscard]] core::Result<std::unique_ptr<assembly::AssemblyRuntimeGraph>>
deserializeOccurrenceGraph(
    std::span<const std::uint8_t> bytes,
    std::shared_ptr<const assembly::DefinitionRegistry> registry,
    StorageLimits limits = {});

core::Result<bool> savePartContainer(
    const std::filesystem::path &path, const SourceDocumentMetadata &metadata,
    const assembly::DefinitionRegistry &registry,
    std::span<const OpaqueAuthoritativePayload> opaquePayloads = {},
    bool simulateFailureBeforeReplace = false);
core::Result<bool> saveAssemblyContainer(
    const std::filesystem::path &path, const SourceDocumentMetadata &metadata,
    const assembly::DefinitionRegistry &registry,
    const assembly::AssemblyRuntimeGraph &graph,
    std::span<const OpaqueAuthoritativePayload> opaquePayloads = {},
    bool simulateFailureBeforeReplace = false);
core::Result<SourceDocumentMetadata>
readDocumentMetadata(const DuomecContainerReader &reader,
                     StorageLimits limits = {});
core::Result<std::shared_ptr<assembly::DefinitionRegistry>>
readDefinitionRegistry(const DuomecContainerReader &reader,
                       StorageLimits limits = {});
core::Result<LoadedAssembly>
loadAssemblyContainer(const std::filesystem::path &path,
                      StorageLimits limits = {});

} // namespace duomec::storage

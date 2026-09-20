#pragma once

#include "duomec/cad/document/ids.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace duomec::storage {

enum class ContainerType : std::uint8_t { Part = 1, Assembly = 2 };
enum class ChunkType : std::uint32_t {
  DocumentMetadata = 1,
  DefinitionRegistry = 2,
  RevisionManifest = 3,
  AssemblyOccurrenceGraph = 4,
  AssemblyRelations = 5,
  AuthoringPayload = 6,
  ExactGeometryPayload = 7,
  VirtualDefinitions = 8
};
enum class ChunkCodec : std::uint8_t { None = 0, Zstd = 1 };
enum class ChunkCriticality : std::uint8_t { Optional = 0, Required = 1 };

struct StorageLimits {
  std::uint32_t maxChunkCount{65'536};
  std::uint64_t maxChunkSize{4ULL * 1024 * 1024 * 1024};
  std::uint64_t maxDecompressedChunkSize{8ULL * 1024 * 1024 * 1024};
  std::uint32_t maxManifestEntries{2'000'000};
  std::uint32_t maxOccurrenceRecords{5'000'000};
  std::uint32_t maxRelationRecords{5'000'000};
  std::uint32_t maxMetadataStringSize{16 * 1024 * 1024};
};

struct ContainerHeader {
  static constexpr std::uint16_t currentMajor = 1;
  static constexpr std::uint16_t currentMinor = 0;
  std::uint16_t major{currentMajor};
  std::uint16_t minor{currentMinor};
  ContainerType type{ContainerType::Part};
  std::uint32_t flags{};
  std::uint64_t tocOffset{};
  std::uint32_t tocEntryCount{};
  cad::DocumentId documentId{cad::DocumentId::generate()};
};

struct ChunkDescriptor {
  std::string id;
  ChunkType type{ChunkType::DocumentMetadata};
  std::uint32_t schemaVersion{1};
  ChunkCodec codec{ChunkCodec::None};
  ChunkCriticality criticality{ChunkCriticality::Required};
  std::uint64_t payloadOffset{};
  std::uint64_t storedSize{};
  std::uint64_t uncompressedSize{};
  std::uint32_t checksum{};
  std::string semanticContentHash;
  auto operator<=>(const ChunkDescriptor &) const = default;
};

struct ChunkWriteRequest {
  std::string id;
  ChunkType type{ChunkType::DocumentMetadata};
  std::uint32_t schemaVersion{1};
  ChunkCodec codec{ChunkCodec::None};
  ChunkCriticality criticality{ChunkCriticality::Required};
  std::string semanticContentHash;
  std::vector<std::uint8_t> payload;
};

[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes);

class DuomecContainerWriter {
public:
  explicit DuomecContainerWriter(
      ContainerType type,
      cad::DocumentId documentId = cad::DocumentId::generate());
  core::Result<bool> addChunk(ChunkWriteRequest chunk);
  core::Result<bool>
  writeAtomic(const std::filesystem::path &target,
              bool simulateFailureBeforeReplace = false) const;

private:
  ContainerHeader header_;
  std::vector<ChunkWriteRequest> chunks_;
};

class DuomecContainerReader {
public:
  static core::Result<DuomecContainerReader>
  open(const std::filesystem::path &path, StorageLimits limits = {});
  [[nodiscard]] const ContainerHeader &header() const noexcept {
    return header_;
  }
  [[nodiscard]] const std::vector<ChunkDescriptor> &
  listChunks() const noexcept {
    return chunks_;
  }
  [[nodiscard]] const ChunkDescriptor *findChunk(std::string_view id) const;
  [[nodiscard]] const ChunkDescriptor *findChunk(ChunkType type) const;
  core::Result<std::vector<std::uint8_t>> readChunk(std::string_view id) const;
  core::Result<std::vector<std::uint8_t>> readChunk(ChunkType type) const;
  [[nodiscard]] std::uint64_t bytesRead() const noexcept { return bytesRead_; }
  [[nodiscard]] std::uint64_t fileSize() const noexcept { return fileSize_; }

private:
  std::filesystem::path path_;
  StorageLimits limits_;
  ContainerHeader header_;
  std::vector<ChunkDescriptor> chunks_;
  std::uint64_t fileSize_{};
  mutable std::uint64_t bytesRead_{};
};

} // namespace duomec::storage

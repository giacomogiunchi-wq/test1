#include "duomec/storage/container.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <system_error>
#ifdef DUOMEC_ENABLE_ZSTD
#include <zstd.h>
#endif
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace duomec::storage {
namespace {
constexpr std::array<std::uint8_t, 8> magic{'D', 'U', 'O', 'M',
                                            'E', 'C', 'C', '1'};
constexpr std::uint8_t littleEndianMarker = 1;
constexpr std::uint64_t headerSize = 72;
constexpr std::uint64_t descriptorSize = 144;

core::Error error(core::ErrorCode code, std::string message,
                  std::string context = {}) {
  return {code, std::move(message), std::move(context)};
}
template <class Integer> void put(std::ostream &out, Integer value) {
  using Unsigned = std::make_unsigned_t<Integer>;
  auto converted = static_cast<Unsigned>(value);
  for (std::size_t index = 0; index < sizeof(Integer); ++index)
    out.put(static_cast<char>((converted >> (index * 8U)) & 0xffU));
}
template <class Integer> bool get(std::istream &in, Integer &value) {
  std::uint64_t converted{};
  for (std::size_t index = 0; index < sizeof(Integer); ++index) {
    const auto byte = in.get();
    if (byte == std::char_traits<char>::eof())
      return false;
    converted |= static_cast<std::uint64_t>(static_cast<unsigned char>(byte))
                 << (index * 8U);
  }
  value = static_cast<Integer>(converted);
  return true;
}
void fixed(std::ostream &out, std::string_view value, std::size_t size) {
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
  for (std::size_t index = value.size(); index < size; ++index)
    out.put('\0');
}
bool readFixed(std::istream &in, std::string &value, std::size_t size) {
  std::string buffer(size, '\0');
  if (!in.read(buffer.data(), static_cast<std::streamsize>(size)))
    return false;
  const auto end = buffer.find('\0');
  value.assign(buffer.data(), end == std::string::npos ? size : end);
  return true;
}
bool known(ChunkType type) {
  const auto value = static_cast<std::uint32_t>(type);
  return value >= static_cast<std::uint32_t>(ChunkType::DocumentMetadata) &&
         value <= static_cast<std::uint32_t>(ChunkType::VirtualDefinitions);
}
bool addOverflow(std::uint64_t left, std::uint64_t right,
                 std::uint64_t &result) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left)
    return true;
  result = left + right;
  return false;
}
void writeHeader(std::ostream &out, const ContainerHeader &header) {
  out.write(reinterpret_cast<const char *>(magic.data()), magic.size());
  put(out, header.major);
  put(out, header.minor);
  put(out, static_cast<std::uint8_t>(header.type));
  put(out, littleEndianMarker);
  put<std::uint16_t>(out, 0);
  put(out, header.flags);
  put(out, header.tocOffset);
  put(out, header.tocEntryCount);
  fixed(out, header.documentId.value(), 36);
  put<std::uint32_t>(out, 0);
}
void writeDescriptor(std::ostream &out, const ChunkDescriptor &descriptor) {
  fixed(out, descriptor.id, 36);
  put(out, static_cast<std::uint32_t>(descriptor.type));
  put(out, descriptor.schemaVersion);
  put(out, static_cast<std::uint8_t>(descriptor.codec));
  put(out, static_cast<std::uint8_t>(descriptor.criticality));
  put<std::uint16_t>(out, 0);
  put(out, descriptor.payloadOffset);
  put(out, descriptor.storedSize);
  put(out, descriptor.uncompressedSize);
  put(out, descriptor.checksum);
  put(out, static_cast<std::uint16_t>(descriptor.semanticContentHash.size()));
  fixed(out, descriptor.semanticContentHash, 64);
  put<std::uint16_t>(out, 0);
}
core::Result<bool> replace(const std::filesystem::path &temporary,
                           const std::filesystem::path &target) {
  std::error_code ec;
#ifdef _WIN32
  std::filesystem::remove(target, ec);
  ec.clear();
#endif
  std::filesystem::rename(temporary, target, ec);
  if (ec)
    return core::Result<bool>::failure(
        error(core::ErrorCode::io_failure, "atomic replacement failed",
              target.string() + ": " + ec.message()));
  return core::Result<bool>::success(true);
}
} // namespace

std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
  std::uint32_t crc = 0xffffffffU;
  for (const auto byte : bytes) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return ~crc;
}

DuomecContainerWriter::DuomecContainerWriter(ContainerType type,
                                             cad::DocumentId documentId) {
  header_.type = type;
  header_.documentId = std::move(documentId);
}
core::Result<bool> DuomecContainerWriter::addChunk(ChunkWriteRequest chunk) {
  if (chunk.id.empty() || chunk.id.size() > 36 ||
      chunk.semanticContentHash.size() > 64)
    return core::Result<bool>::failure(
        error(core::ErrorCode::invalid_argument,
              "invalid chunk key or content hash", chunk.id));
  if (std::any_of(chunks_.begin(), chunks_.end(),
                  [&](const auto &value) { return value.id == chunk.id; }))
    return core::Result<bool>::failure(error(core::ErrorCode::invalid_argument,
                                             "duplicate chunk id", chunk.id));
  if (chunk.codec == ChunkCodec::Zstd) {
#ifndef DUOMEC_ENABLE_ZSTD
    return core::Result<bool>::failure(
        error(core::ErrorCode::dependency_missing,
              "zstd codec adapter is not enabled in this build", chunk.id));
#endif
  }
  chunks_.push_back(std::move(chunk));
  return core::Result<bool>::success(true);
}
core::Result<bool>
DuomecContainerWriter::writeAtomic(const std::filesystem::path &target,
                                   bool simulateFailureBeforeReplace) const {
  const auto temporary =
      target.parent_path() / (target.filename().string() + ".tmp-" +
                              cad::DocumentId::generate().value());
  std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
  if (!out)
    return core::Result<bool>::failure(
        error(core::ErrorCode::io_failure, "cannot create temporary container",
              temporary.string()));
  ContainerHeader header = header_;
  writeHeader(out, header);
  std::vector<ChunkDescriptor> descriptors;
  descriptors.reserve(chunks_.size());
  for (const auto &chunk : chunks_) {
    std::vector<std::uint8_t> stored = chunk.payload;
#ifdef DUOMEC_ENABLE_ZSTD
    if (chunk.codec == ChunkCodec::Zstd) {
      stored.resize(ZSTD_compressBound(chunk.payload.size()));
      const auto compressed =
          ZSTD_compress(stored.data(), stored.size(), chunk.payload.data(),
                        chunk.payload.size(), 3);
      if (ZSTD_isError(compressed)) {
        std::filesystem::remove(temporary);
        return core::Result<bool>::failure(error(core::ErrorCode::io_failure,
                                                 ZSTD_getErrorName(compressed),
                                                 chunk.id));
      }
      stored.resize(compressed);
    }
#endif
    ChunkDescriptor descriptor;
    descriptor.id = chunk.id;
    descriptor.type = chunk.type;
    descriptor.schemaVersion = chunk.schemaVersion;
    descriptor.codec = chunk.codec;
    descriptor.criticality = chunk.criticality;
    descriptor.payloadOffset = static_cast<std::uint64_t>(out.tellp());
    descriptor.storedSize = stored.size();
    descriptor.uncompressedSize = chunk.payload.size();
    descriptor.checksum = crc32(chunk.payload);
    descriptor.semanticContentHash = chunk.semanticContentHash;
    out.write(reinterpret_cast<const char *>(stored.data()),
              static_cast<std::streamsize>(stored.size()));
    descriptors.push_back(std::move(descriptor));
  }
  header.tocOffset = static_cast<std::uint64_t>(out.tellp());
  header.tocEntryCount = static_cast<std::uint32_t>(descriptors.size());
  for (const auto &descriptor : descriptors)
    writeDescriptor(out, descriptor);
  out.seekp(0);
  writeHeader(out, header);
  out.flush();
  out.close();
  if (!out) {
    std::filesystem::remove(temporary);
    return core::Result<bool>::failure(error(core::ErrorCode::io_failure,
                                             "container write failed",
                                             temporary.string()));
  }
#ifndef _WIN32
  const auto fd = ::open(temporary.c_str(), O_RDONLY);
  if (fd >= 0) {
    (void)::fsync(fd);
    (void)::close(fd);
  }
#endif
  const auto opened = DuomecContainerReader::open(temporary);
  if (!opened) {
    std::filesystem::remove(temporary);
    return core::Result<bool>::failure(opened.error());
  }
  for (const auto &descriptor : opened.value().listChunks()) {
    if (!known(descriptor.type) &&
        descriptor.criticality == ChunkCriticality::Optional)
      continue;
    const auto checked = opened.value().readChunk(descriptor.id);
    if (!checked) {
      std::filesystem::remove(temporary);
      return core::Result<bool>::failure(checked.error());
    }
  }
  if (simulateFailureBeforeReplace) {
    std::filesystem::remove(temporary);
    return core::Result<bool>::failure(
        error(core::ErrorCode::io_failure,
              "simulated failure before replacement", target.string()));
  }
  auto result = replace(temporary, target);
  if (!result)
    std::filesystem::remove(temporary);
  return result;
}

core::Result<DuomecContainerReader>
DuomecContainerReader::open(const std::filesystem::path &path,
                            StorageLimits limits) {
  std::error_code ec;
  const auto fileSize = std::filesystem::file_size(path, ec);
  if (ec || fileSize < headerSize)
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::io_failure, "container is missing or truncated",
              path.string()));
  std::ifstream in(path, std::ios::binary);
  std::array<std::uint8_t, 8> actualMagic{};
  in.read(reinterpret_cast<char *>(actualMagic.data()), actualMagic.size());
  if (!in || actualMagic != magic)
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::io_failure, "invalid Duomec container magic",
              path.string()));
  DuomecContainerReader reader;
  reader.path_ = path;
  reader.limits_ = limits;
  reader.fileSize_ = fileSize;
  std::uint8_t type{}, endian{};
  std::uint16_t reserved{};
  std::uint32_t headerChecksum{};
  std::string documentId;
  if (!get(in, reader.header_.major) || !get(in, reader.header_.minor) ||
      !get(in, type) || !get(in, endian) || !get(in, reserved) ||
      !get(in, reader.header_.flags) || !get(in, reader.header_.tocOffset) ||
      !get(in, reader.header_.tocEntryCount) ||
      !readFixed(in, documentId, 36) || !get(in, headerChecksum))
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::io_failure, "truncated container header",
              path.string()));
  (void)reserved;
  (void)headerChecksum;
  if (reader.header_.major != ContainerHeader::currentMajor)
    return core::Result<DuomecContainerReader>::failure(error(
        core::ErrorCode::unsupported, "unsupported container major version",
        std::to_string(reader.header_.major)));
  if (endian != littleEndianMarker || (type != 1 && type != 2))
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::unsupported, "unsupported container encoding",
              path.string()));
  reader.header_.type = static_cast<ContainerType>(type);
  const auto parsedId = cad::DocumentId::parse(documentId);
  if (!parsedId)
    return core::Result<DuomecContainerReader>::failure(parsedId.error());
  reader.header_.documentId = parsedId.value();
  if (reader.header_.tocEntryCount > limits.maxChunkCount)
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::io_failure, "chunk count exceeds safety limit",
              path.string()));
  std::uint64_t tocSize{};
  if (reader.header_.tocEntryCount >
      std::numeric_limits<std::uint64_t>::max() / descriptorSize)
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::io_failure, "TOC size overflow", path.string()));
  tocSize = reader.header_.tocEntryCount * descriptorSize;
  std::uint64_t tocEnd{};
  if (reader.header_.tocOffset < headerSize ||
      addOverflow(reader.header_.tocOffset, tocSize, tocEnd) ||
      tocEnd > fileSize)
    return core::Result<DuomecContainerReader>::failure(
        error(core::ErrorCode::io_failure, "invalid table of contents range",
              path.string()));
  in.seekg(static_cast<std::streamoff>(reader.header_.tocOffset));
  std::set<std::string, std::less<>> ids;
  for (std::uint32_t index = 0; index < reader.header_.tocEntryCount; ++index) {
    ChunkDescriptor descriptor;
    std::uint32_t chunkType{};
    std::uint8_t codec{}, criticality{};
    std::uint16_t contentHashLength{}, padding{};
    std::string hashField;
    if (!readFixed(in, descriptor.id, 36) || !get(in, chunkType) ||
        !get(in, descriptor.schemaVersion) || !get(in, codec) ||
        !get(in, criticality) || !get(in, padding) ||
        !get(in, descriptor.payloadOffset) || !get(in, descriptor.storedSize) ||
        !get(in, descriptor.uncompressedSize) ||
        !get(in, descriptor.checksum) || !get(in, contentHashLength) ||
        !readFixed(in, hashField, 64) || !get(in, padding))
      return core::Result<DuomecContainerReader>::failure(
          error(core::ErrorCode::io_failure, "truncated table of contents",
                path.string()));
    if (descriptor.id.empty() || !ids.insert(descriptor.id).second ||
        contentHashLength > 64 || hashField.size() < contentHashLength)
      return core::Result<DuomecContainerReader>::failure(
          error(core::ErrorCode::io_failure,
                "invalid or duplicate chunk descriptor", descriptor.id));
    descriptor.type = static_cast<ChunkType>(chunkType);
    descriptor.codec = static_cast<ChunkCodec>(codec);
    descriptor.criticality = static_cast<ChunkCriticality>(criticality);
    descriptor.semanticContentHash = hashField.substr(0, contentHashLength);
    std::uint64_t payloadEnd{};
    if (descriptor.storedSize > limits.maxChunkSize ||
        descriptor.uncompressedSize > limits.maxDecompressedChunkSize ||
        descriptor.payloadOffset < headerSize ||
        addOverflow(descriptor.payloadOffset, descriptor.storedSize,
                    payloadEnd) ||
        payloadEnd > reader.header_.tocOffset)
      return core::Result<DuomecContainerReader>::failure(error(
          core::ErrorCode::io_failure, "invalid chunk range", descriptor.id));
    if (!known(descriptor.type) &&
        descriptor.criticality == ChunkCriticality::Required)
      return core::Result<DuomecContainerReader>::failure(
          error(core::ErrorCode::unsupported, "unknown required chunk type",
                descriptor.id));
    if (known(descriptor.type) && descriptor.codec != ChunkCodec::None &&
        descriptor.codec != ChunkCodec::Zstd)
      return core::Result<DuomecContainerReader>::failure(
          error(core::ErrorCode::dependency_missing, "chunk codec unavailable",
                descriptor.id));
    reader.chunks_.push_back(std::move(descriptor));
  }
  auto ranges = reader.chunks_;
  std::sort(ranges.begin(), ranges.end(),
            [](const auto &left, const auto &right) {
              return left.payloadOffset < right.payloadOffset;
            });
  for (std::size_t index = 1; index < ranges.size(); ++index) {
    std::uint64_t previousEnd{};
    if (addOverflow(ranges[index - 1].payloadOffset,
                    ranges[index - 1].storedSize, previousEnd) ||
        previousEnd > ranges[index].payloadOffset)
      return core::Result<DuomecContainerReader>::failure(
          error(core::ErrorCode::io_failure, "overlapping chunk ranges",
                ranges[index].id));
  }
  reader.bytesRead_ = headerSize + tocSize;
  return core::Result<DuomecContainerReader>::success(std::move(reader));
}
const ChunkDescriptor *
DuomecContainerReader::findChunk(std::string_view id) const {
  const auto found =
      std::find_if(chunks_.begin(), chunks_.end(),
                   [&](const auto &value) { return value.id == id; });
  return found == chunks_.end() ? nullptr : &*found;
}
const ChunkDescriptor *DuomecContainerReader::findChunk(ChunkType type) const {
  const auto found =
      std::find_if(chunks_.begin(), chunks_.end(),
                   [&](const auto &value) { return value.type == type; });
  return found == chunks_.end() ? nullptr : &*found;
}
core::Result<std::vector<std::uint8_t>>
DuomecContainerReader::readChunk(std::string_view id) const {
  const auto *descriptor = findChunk(id);
  if (!descriptor)
    return core::Result<std::vector<std::uint8_t>>::failure(error(
        core::ErrorCode::invalid_argument, "chunk not found", std::string(id)));
  if (descriptor->codec != ChunkCodec::None &&
      descriptor->codec != ChunkCodec::Zstd)
    return core::Result<std::vector<std::uint8_t>>::failure(
        error(core::ErrorCode::dependency_missing, "chunk codec unavailable",
              descriptor->id));
  std::ifstream in(path_, std::ios::binary);
  in.seekg(static_cast<std::streamoff>(descriptor->payloadOffset));
  std::vector<std::uint8_t> stored(
      static_cast<std::size_t>(descriptor->storedSize));
  if (!in.read(reinterpret_cast<char *>(stored.data()),
               static_cast<std::streamsize>(stored.size())))
    return core::Result<std::vector<std::uint8_t>>::failure(
        error(core::ErrorCode::io_failure, "truncated chunk payload",
              descriptor->id));
  bytesRead_ += stored.size();
  std::vector<std::uint8_t> bytes;
  if (descriptor->codec == ChunkCodec::Zstd) {
#ifdef DUOMEC_ENABLE_ZSTD
    bytes.resize(static_cast<std::size_t>(descriptor->uncompressedSize));
    const auto decompressed = ZSTD_decompress(bytes.data(), bytes.size(),
                                              stored.data(), stored.size());
    if (ZSTD_isError(decompressed) || decompressed != bytes.size())
      return core::Result<std::vector<std::uint8_t>>::failure(
          error(core::ErrorCode::io_failure, "zstd chunk decompression failed",
                descriptor->id));
#else
    return core::Result<std::vector<std::uint8_t>>::failure(error(
        core::ErrorCode::dependency_missing,
        "zstd codec adapter is not enabled in this build", descriptor->id));
#endif
  } else {
    bytes = std::move(stored);
  }
  if (crc32(bytes) != descriptor->checksum)
    return core::Result<std::vector<std::uint8_t>>::failure(
        error(core::ErrorCode::io_failure, "chunk checksum mismatch",
              descriptor->id));
  return core::Result<std::vector<std::uint8_t>>::success(std::move(bytes));
}
core::Result<std::vector<std::uint8_t>>
DuomecContainerReader::readChunk(ChunkType type) const {
  const auto *descriptor = findChunk(type);
  if (!descriptor)
    return core::Result<std::vector<std::uint8_t>>::failure(
        error(core::ErrorCode::invalid_argument, "chunk type not found",
              std::to_string(static_cast<std::uint32_t>(type))));
  return readChunk(descriptor->id);
}

} // namespace duomec::storage

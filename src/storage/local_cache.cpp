#include "duomec/storage/local_cache.hpp"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace duomec::storage {
namespace {
std::string kindName(AssetKind kind) {
  switch (kind) {
  case AssetKind::Mesh:
    return "mesh";
  case AssetKind::EdgeGeometry:
    return "edge";
  case AssetKind::Selection:
    return "selection";
  case AssetKind::Bvh:
    return "bvh";
  case AssetKind::Lod:
    return "lod";
  case AssetKind::Proxy:
    return "proxy";
  case AssetKind::Analysis:
    return "analysis";
  }
  return "unknown";
}
std::string safe(std::string_view value) {
  std::ostringstream out;
  for (const auto character : value) {
    const auto byte = static_cast<unsigned char>(character);
    if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9') || byte == '-' || byte == '_')
      out << character;
    else
      out << '_' << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<unsigned>(byte);
  }
  return out.str();
}
std::uint64_t now() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}
template <class Integer> void writeInt(std::ostream &out, Integer value) {
  using Unsigned = std::make_unsigned_t<Integer>;
  auto converted = static_cast<Unsigned>(value);
  for (std::size_t index = 0; index < sizeof(Integer); ++index)
    out.put(static_cast<char>((converted >> (index * 8U)) & 0xffU));
}
template <class Integer> bool readInt(std::istream &in, Integer &value) {
  using Unsigned = std::make_unsigned_t<Integer>;
  Unsigned result{};
  for (std::size_t index = 0; index < sizeof(Integer); ++index) {
    const auto byte = in.get();
    if (byte == std::char_traits<char>::eof())
      return false;
    result |= static_cast<Unsigned>(static_cast<unsigned char>(byte))
              << (index * 8U);
  }
  value = static_cast<Integer>(result);
  return true;
}
void string(std::ostream &out, std::string_view value) {
  writeInt<std::uint32_t>(out, static_cast<std::uint32_t>(value.size()));
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
}
bool string(std::istream &in, std::string &value) {
  std::uint32_t size{};
  if (!readInt(in, size) || size > 1024 * 1024)
    return false;
  value.resize(size);
  return static_cast<bool>(in.read(value.data(), size));
}
} // namespace

void InMemoryCacheCatalog::upsert(CacheCatalogEntry entry) {
  std::scoped_lock lock(mutex_);
  entries_.insert_or_assign(entry.key, std::move(entry));
}
std::optional<CacheCatalogEntry>
InMemoryCacheCatalog::find(const CacheKey &key) const {
  std::scoped_lock lock(mutex_);
  const auto found = entries_.find(key);
  return found == entries_.end()
             ? std::nullopt
             : std::optional<CacheCatalogEntry>(found->second);
}
void InMemoryCacheCatalog::remove(const CacheKey &key) {
  std::scoped_lock lock(mutex_);
  entries_.erase(key);
}
void InMemoryCacheCatalog::clear() {
  std::scoped_lock lock(mutex_);
  entries_.clear();
}
std::size_t InMemoryCacheCatalog::size() const {
  std::scoped_lock lock(mutex_);
  return entries_.size();
}

LocalAssetCache::LocalAssetCache(std::filesystem::path root,
                                 std::shared_ptr<ICacheCatalog> catalog)
    : root_(std::move(root)), catalog_(std::move(catalog)) {}
std::filesystem::path LocalAssetCache::pathFor(const CacheKey &key) const {
  const auto hash = safe(key.contentHash);
  const auto first = hash.size() >= 2 ? hash.substr(0, 2) : "__";
  const auto second = hash.size() >= 4 ? hash.substr(2, 2) : "__";
  const auto settingsChecksum = crc32(std::span<const std::uint8_t>(
      reinterpret_cast<const std::uint8_t *>(key.settingsIdentity.data()),
      key.settingsIdentity.size()));
  return root_ / kindName(key.kind) / first / second /
         (hash + "-f" + std::to_string(key.formatVersion) + "-g" +
          safe(key.generatorVersion) + "-s" + std::to_string(settingsChecksum) +
          ".asset");
}
core::Result<bool> LocalAssetCache::put(const CacheKey &key,
                                        std::span<const std::uint8_t> bytes) {
  std::unique_lock lock(mutex_);
  const auto path = pathFor(key);
  if (std::filesystem::exists(path)) {
    lock.unlock();
    auto existing = get(key);
    if (existing && existing.value())
      return core::Result<bool>::success(false);
    lock.lock();
  }
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec)
    return core::Result<bool>::failure(
        {core::ErrorCode::io_failure, ec.message(), path.string()});
  const auto temporary =
      path.string() + ".tmp-" + cad::DocumentId::generate().value();
  std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
  const std::array<char, 8> magic{'D', 'U', 'O', 'C', 'A', 'C', 'H', '1'};
  out.write(magic.data(), magic.size());
  writeInt(out, static_cast<std::uint32_t>(key.kind));
  writeInt(out, key.formatVersion);
  string(out, key.contentHash);
  string(out, key.generatorVersion);
  string(out, key.settingsIdentity);
  writeInt(out, static_cast<std::uint64_t>(bytes.size()));
  writeInt(out, crc32(bytes));
  out.write(reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  out.flush();
  out.close();
  if (!out) {
    std::filesystem::remove(temporary);
    return core::Result<bool>::failure({core::ErrorCode::io_failure,
                                        "cache entry write failed",
                                        path.string()});
  }
  std::filesystem::rename(temporary, path, ec);
  if (ec) {
    std::filesystem::remove(temporary);
    return core::Result<bool>::failure(
        {core::ErrorCode::io_failure, ec.message(), path.string()});
  }
  const auto timestamp = now();
  catalog_->upsert({key, path, bytes.size(), timestamp, timestamp, true});
  statistics_.entries = catalog_->size();
  statistics_.storedBytes += bytes.size();
  return core::Result<bool>::success(true);
}
core::Result<std::optional<std::vector<std::uint8_t>>>
LocalAssetCache::get(const CacheKey &key) {
  std::scoped_lock lock(mutex_);
  const auto path = pathFor(key);
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    ++statistics_.misses;
    return core::Result<std::optional<std::vector<std::uint8_t>>>::success(
        std::nullopt);
  }
  std::array<char, 8> magic{};
  std::uint32_t kind{}, format{}, checksum{};
  std::uint64_t size{};
  std::string contentHash, generator, settings;
  const std::array<char, 8> expected{'D', 'U', 'O', 'C', 'A', 'C', 'H', '1'};
  if (!in.read(magic.data(), magic.size()) || magic != expected ||
      !readInt(in, kind) || !readInt(in, format) || !string(in, contentHash) ||
      !string(in, generator) || !string(in, settings) || !readInt(in, size) ||
      !readInt(in, checksum) || size > 4ULL * 1024 * 1024 * 1024 ||
      kind != static_cast<std::uint32_t>(key.kind) ||
      format != key.formatVersion || contentHash != key.contentHash ||
      generator != key.generatorVersion || settings != key.settingsIdentity) {
    ++statistics_.corruptions;
    ++statistics_.misses;
    in.close();
    std::filesystem::remove(path);
    catalog_->remove(key);
    return core::Result<std::optional<std::vector<std::uint8_t>>>::success(
        std::nullopt);
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  if (!in.read(reinterpret_cast<char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size())) ||
      crc32(bytes) != checksum) {
    ++statistics_.corruptions;
    ++statistics_.misses;
    in.close();
    std::filesystem::remove(path);
    catalog_->remove(key);
    return core::Result<std::optional<std::vector<std::uint8_t>>>::success(
        std::nullopt);
  }
  auto entry = catalog_->find(key).value_or(
      CacheCatalogEntry{key, path, bytes.size(), now(), now(), true});
  entry.lastAccessUnixSeconds = now();
  catalog_->upsert(std::move(entry));
  ++statistics_.hits;
  return core::Result<std::optional<std::vector<std::uint8_t>>>::success(
      std::move(bytes));
}
bool LocalAssetCache::validate(const CacheKey &key) {
  const auto result = get(key);
  return result && result.value().has_value();
}
core::Result<bool> LocalAssetCache::remove(const CacheKey &key) {
  std::scoped_lock lock(mutex_);
  std::error_code ec;
  const auto removed = std::filesystem::remove(pathFor(key), ec);
  if (ec)
    return core::Result<bool>::failure(
        {core::ErrorCode::io_failure, ec.message(), pathFor(key).string()});
  catalog_->remove(key);
  statistics_.entries = catalog_->size();
  return core::Result<bool>::success(removed);
}
core::Result<bool> LocalAssetCache::clear() {
  std::scoped_lock lock(mutex_);
  std::error_code ec;
  std::filesystem::remove_all(root_, ec);
  if (ec)
    return core::Result<bool>::failure(
        {core::ErrorCode::io_failure, ec.message(), root_.string()});
  catalog_->clear();
  statistics_.entries = 0;
  statistics_.storedBytes = 0;
  return core::Result<bool>::success(true);
}
CacheStatistics LocalAssetCache::statistics() const {
  std::scoped_lock lock(mutex_);
  auto result = statistics_;
  result.entries = catalog_->size();
  return result;
}
CacheKey geometryCacheKey(AssetKind kind, const assembly::GeometryHash &hash,
                          std::uint32_t formatVersion,
                          std::string generatorVersion,
                          std::string settingsIdentity) {
  return {kind, hash.value(), formatVersion, std::move(generatorVersion),
          std::move(settingsIdentity)};
}

} // namespace duomec::storage

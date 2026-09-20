#pragma once

#include "duomec/assembly/runtime_model.hpp"
#include "duomec/storage/container.hpp"

#include <chrono>
#include <mutex>

namespace duomec::storage {

enum class AssetKind {
  Mesh,
  EdgeGeometry,
  Selection,
  Bvh,
  Lod,
  Proxy,
  Analysis
};
struct CacheKey {
  AssetKind kind{AssetKind::Mesh};
  std::string contentHash;
  std::uint32_t formatVersion{1};
  std::string generatorVersion;
  std::string settingsIdentity;
  auto operator<=>(const CacheKey &) const = default;
};
struct CacheCatalogEntry {
  CacheKey key;
  std::filesystem::path location;
  std::uint64_t storedSize{};
  std::uint64_t createdUnixSeconds{};
  std::uint64_t lastAccessUnixSeconds{};
  bool valid{true};
};
class ICacheCatalog {
public:
  virtual ~ICacheCatalog() = default;
  virtual void upsert(CacheCatalogEntry entry) = 0;
  virtual std::optional<CacheCatalogEntry> find(const CacheKey &key) const = 0;
  virtual void remove(const CacheKey &key) = 0;
  virtual void clear() = 0;
  virtual std::size_t size() const = 0;
};
class InMemoryCacheCatalog final : public ICacheCatalog {
public:
  void upsert(CacheCatalogEntry entry) override;
  std::optional<CacheCatalogEntry> find(const CacheKey &key) const override;
  void remove(const CacheKey &key) override;
  void clear() override;
  std::size_t size() const override;

private:
  mutable std::mutex mutex_;
  std::map<CacheKey, CacheCatalogEntry> entries_;
};
struct CacheStatistics {
  std::uint64_t entries{};
  std::uint64_t storedBytes{};
  std::uint64_t hits{};
  std::uint64_t misses{};
  std::uint64_t corruptions{};
};
class LocalAssetCache {
public:
  LocalAssetCache(std::filesystem::path root,
                  std::shared_ptr<ICacheCatalog> catalog);
  [[nodiscard]] std::filesystem::path pathFor(const CacheKey &key) const;
  core::Result<bool> put(const CacheKey &key,
                         std::span<const std::uint8_t> bytes);
  core::Result<std::optional<std::vector<std::uint8_t>>>
  get(const CacheKey &key);
  [[nodiscard]] bool validate(const CacheKey &key);
  core::Result<bool> remove(const CacheKey &key);
  core::Result<bool> clear();
  [[nodiscard]] CacheStatistics statistics() const;

private:
  std::filesystem::path root_;
  std::shared_ptr<ICacheCatalog> catalog_;
  mutable std::mutex mutex_;
  CacheStatistics statistics_;
};

[[nodiscard]] CacheKey geometryCacheKey(AssetKind kind,
                                        const assembly::GeometryHash &hash,
                                        std::uint32_t formatVersion,
                                        std::string generatorVersion,
                                        std::string settingsIdentity = {});

} // namespace duomec::storage

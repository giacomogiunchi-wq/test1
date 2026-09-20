#pragma once

#include "duomec/storage/local_cache.hpp"

#ifdef DUOMEC_ENABLE_SQLITE_CACHE
namespace duomec::storage {

class SqliteCacheCatalog final : public ICacheCatalog {
public:
  explicit SqliteCacheCatalog(const std::filesystem::path &database);
  ~SqliteCacheCatalog() override;
  SqliteCacheCatalog(const SqliteCacheCatalog &) = delete;
  SqliteCacheCatalog &operator=(const SqliteCacheCatalog &) = delete;
  void upsert(CacheCatalogEntry entry) override;
  std::optional<CacheCatalogEntry> find(const CacheKey &key) const override;
  void remove(const CacheKey &key) override;
  void clear() override;
  std::size_t size() const override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace duomec::storage
#endif

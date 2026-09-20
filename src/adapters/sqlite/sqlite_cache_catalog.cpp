#include "duomec/storage/sqlite_cache_catalog.hpp"

#include <sqlite3.h>

#include <mutex>
#include <stdexcept>

namespace duomec::storage {
namespace {
void check(int status, sqlite3 *database) {
  if (status != SQLITE_OK && status != SQLITE_DONE && status != SQLITE_ROW)
    throw std::runtime_error(sqlite3_errmsg(database));
}
void bindKey(sqlite3_stmt *statement, const CacheKey &key, int first = 1) {
  sqlite3_bind_int(statement, first, static_cast<int>(key.kind));
  sqlite3_bind_text(statement, first + 1, key.contentHash.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(statement, first + 2, key.formatVersion);
  sqlite3_bind_text(statement, first + 3, key.generatorVersion.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, first + 4, key.settingsIdentity.c_str(), -1,
                    SQLITE_TRANSIENT);
}
} // namespace

class SqliteCacheCatalog::Impl {
public:
  explicit Impl(const std::filesystem::path &path) {
    if (sqlite3_open(path.string().c_str(), &database) != SQLITE_OK)
      throw std::runtime_error("cannot open SQLite cache catalog");
    check(sqlite3_exec(database, "PRAGMA journal_mode=WAL", nullptr, nullptr,
                       nullptr),
          database);
    check(sqlite3_exec(
              database,
              "CREATE TABLE IF NOT EXISTS cache_entries("
              "kind INTEGER NOT NULL,content_hash TEXT NOT NULL,"
              "format_version INTEGER NOT NULL,generator TEXT NOT NULL,"
              "settings TEXT NOT NULL,location TEXT NOT NULL,stored_size "
              "INTEGER NOT NULL,created INTEGER NOT NULL,last_access INTEGER "
              "NOT NULL,valid INTEGER NOT NULL,PRIMARY KEY(kind,content_hash,"
              "format_version,generator,settings))",
              nullptr, nullptr, nullptr),
          database);
  }
  ~Impl() { sqlite3_close(database); }
  sqlite3 *database{};
  mutable std::mutex mutex;
};

SqliteCacheCatalog::SqliteCacheCatalog(const std::filesystem::path &database)
    : impl_(std::make_unique<Impl>(database)) {}
SqliteCacheCatalog::~SqliteCacheCatalog() = default;
void SqliteCacheCatalog::upsert(CacheCatalogEntry entry) {
  std::scoped_lock lock(impl_->mutex);
  sqlite3_stmt *statement{};
  check(sqlite3_prepare_v2(
            impl_->database,
            "INSERT OR REPLACE INTO cache_entries VALUES(?,?,?,?,?,?,?,?,?,?)",
            -1, &statement, nullptr),
        impl_->database);
  bindKey(statement, entry.key);
  sqlite3_bind_text(statement, 6, entry.location.string().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(statement, 7,
                     static_cast<sqlite3_int64>(entry.storedSize));
  sqlite3_bind_int64(statement, 8,
                     static_cast<sqlite3_int64>(entry.createdUnixSeconds));
  sqlite3_bind_int64(statement, 9,
                     static_cast<sqlite3_int64>(entry.lastAccessUnixSeconds));
  sqlite3_bind_int(statement, 10, entry.valid ? 1 : 0);
  const auto status = sqlite3_step(statement);
  sqlite3_finalize(statement);
  check(status, impl_->database);
}
std::optional<CacheCatalogEntry>
SqliteCacheCatalog::find(const CacheKey &key) const {
  std::scoped_lock lock(impl_->mutex);
  sqlite3_stmt *statement{};
  check(sqlite3_prepare_v2(
            impl_->database,
            "SELECT location,stored_size,created,last_access,valid FROM "
            "cache_entries WHERE kind=? AND content_hash=? AND "
            "format_version=? AND generator=? AND settings=?",
            -1, &statement, nullptr),
        impl_->database);
  bindKey(statement, key);
  const auto status = sqlite3_step(statement);
  if (status == SQLITE_DONE) {
    sqlite3_finalize(statement);
    return std::nullopt;
  }
  check(status, impl_->database);
  CacheCatalogEntry result;
  result.key = key;
  result.location =
      reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
  result.storedSize =
      static_cast<std::uint64_t>(sqlite3_column_int64(statement, 1));
  result.createdUnixSeconds =
      static_cast<std::uint64_t>(sqlite3_column_int64(statement, 2));
  result.lastAccessUnixSeconds =
      static_cast<std::uint64_t>(sqlite3_column_int64(statement, 3));
  result.valid = sqlite3_column_int(statement, 4) != 0;
  sqlite3_finalize(statement);
  return result;
}
void SqliteCacheCatalog::remove(const CacheKey &key) {
  std::scoped_lock lock(impl_->mutex);
  sqlite3_stmt *statement{};
  check(sqlite3_prepare_v2(
            impl_->database,
            "DELETE FROM cache_entries WHERE kind=? AND content_hash=? AND "
            "format_version=? AND generator=? AND settings=?",
            -1, &statement, nullptr),
        impl_->database);
  bindKey(statement, key);
  const auto status = sqlite3_step(statement);
  sqlite3_finalize(statement);
  check(status, impl_->database);
}
void SqliteCacheCatalog::clear() {
  std::scoped_lock lock(impl_->mutex);
  check(sqlite3_exec(impl_->database, "DELETE FROM cache_entries", nullptr,
                     nullptr, nullptr),
        impl_->database);
}
std::size_t SqliteCacheCatalog::size() const {
  std::scoped_lock lock(impl_->mutex);
  sqlite3_stmt *statement{};
  check(sqlite3_prepare_v2(impl_->database,
                           "SELECT COUNT(*) FROM cache_entries", -1, &statement,
                           nullptr),
        impl_->database);
  check(sqlite3_step(statement), impl_->database);
  const auto result =
      static_cast<std::size_t>(sqlite3_column_int64(statement, 0));
  sqlite3_finalize(statement);
  return result;
}

} // namespace duomec::storage

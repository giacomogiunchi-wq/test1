#pragma once

#include "duomec/cad/document/model.hpp"
#include "duomec/core/error.hpp"

#include <filesystem>

namespace duomec::cad {

class IDocumentStore {
public:
  virtual ~IDocumentStore() = default;
  virtual core::Result<bool> initialize(const DocumentSnapshot &snapshot) = 0;
  virtual core::Result<bool> begin_transaction() = 0;
  virtual core::Result<bool> write(const DocumentSnapshot &snapshot) = 0;
  virtual core::Result<bool> commit_transaction() = 0;
  virtual void abort_transaction() noexcept = 0;
  virtual core::Result<bool> can_undo() const = 0;
  virtual core::Result<bool> can_redo() const = 0;
  virtual core::Result<DocumentSnapshot> undo() = 0;
  virtual core::Result<DocumentSnapshot> redo() = 0;
  virtual core::Result<bool> save_atomic(const std::filesystem::path &path) = 0;
  virtual core::Result<DocumentSnapshot>
  load(const std::filesystem::path &path) = 0;
};

} // namespace duomec::cad

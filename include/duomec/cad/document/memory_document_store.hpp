#pragma once

#include "duomec/cad/document/document_store.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace duomec::cad {

struct MemoryDocumentRepository {
  std::unordered_map<std::string, DocumentSnapshot> files;
};

class MemoryDocumentStore final : public IDocumentStore {
public:
  explicit MemoryDocumentStore(
      std::shared_ptr<MemoryDocumentRepository> repository =
          std::make_shared<MemoryDocumentRepository>());
  core::Result<bool> initialize(const DocumentSnapshot &snapshot) override;
  core::Result<bool> begin_transaction() override;
  core::Result<bool> write(const DocumentSnapshot &snapshot) override;
  core::Result<bool> commit_transaction() override;
  void abort_transaction() noexcept override;
  core::Result<bool> can_undo() const override;
  core::Result<bool> can_redo() const override;
  core::Result<DocumentSnapshot> undo() override;
  core::Result<DocumentSnapshot> redo() override;
  core::Result<bool> save_atomic(const std::filesystem::path &path) override;
  core::Result<DocumentSnapshot>
  load(const std::filesystem::path &path) override;

private:
  std::shared_ptr<MemoryDocumentRepository> repository_;
  std::vector<DocumentSnapshot> history_;
  std::size_t cursor_{};
  bool transaction_open_{};
  DocumentSnapshot pending_;
};

} // namespace duomec::cad

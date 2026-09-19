#pragma once

#include "duomec/cad/document/document_store.hpp"

#include <memory>

namespace duomec::adapters::ocaf {

class OcafDocumentStore final : public cad::IDocumentStore {
public:
  OcafDocumentStore();
  ~OcafDocumentStore() override;
  OcafDocumentStore(const OcafDocumentStore &) = delete;
  OcafDocumentStore &operator=(const OcafDocumentStore &) = delete;

  core::Result<bool> initialize(const cad::DocumentSnapshot &snapshot) override;
  core::Result<bool> begin_transaction() override;
  core::Result<bool> write(const cad::DocumentSnapshot &snapshot) override;
  core::Result<bool> commit_transaction() override;
  void abort_transaction() noexcept override;
  core::Result<bool> can_undo() const override;
  core::Result<bool> can_redo() const override;
  core::Result<cad::DocumentSnapshot> undo() override;
  core::Result<cad::DocumentSnapshot> redo() override;
  core::Result<bool> save_atomic(const std::filesystem::path &path) override;
  core::Result<cad::DocumentSnapshot>
  load(const std::filesystem::path &path) override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace duomec::adapters::ocaf

#pragma once

#include "duomec/cad/document/document_store.hpp"

#include <functional>
#include <memory>
#include <string>

namespace duomec::cad {

class ParametricDocument {
public:
  static core::Result<std::unique_ptr<ParametricDocument>>
  create(std::unique_ptr<IDocumentStore> store);
  static core::Result<std::unique_ptr<ParametricDocument>>
  open(std::unique_ptr<IDocumentStore> store,
       const std::filesystem::path &path);

  [[nodiscard]] const DocumentSnapshot &snapshot() const noexcept {
    return snapshot_;
  }
  core::Result<BodyId> add_body(std::string name);
  core::Result<FeatureId> add_generic_feature(BodyId body, std::string name);
  core::Result<ParameterId> add_parameter(BodyId body, FeatureId feature,
                                          std::string name, double value_si);
  core::Result<bool> set_parameter(BodyId body, FeatureId feature,
                                   ParameterId parameter, double value_si);
  core::Result<bool> undo();
  core::Result<bool> redo();
  core::Result<bool> save(const std::filesystem::path &path);

private:
  ParametricDocument(std::unique_ptr<IDocumentStore> store,
                     DocumentSnapshot snapshot);
  core::Result<bool> mutate(
      const std::function<core::Result<bool>(DocumentSnapshot &)> &operation);

  std::unique_ptr<IDocumentStore> store_;
  DocumentSnapshot snapshot_;
};

} // namespace duomec::cad

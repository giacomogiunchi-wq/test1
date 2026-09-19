#include "duomec/cad/document/parametric_document.hpp"
#include "ocaf_document_store.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

int main() {
  using duomec::adapters::ocaf::OcafDocumentStore;
  using duomec::cad::ParametricDocument;
  const auto path =
      std::filesystem::temp_directory_path() / "duomec_m1a_roundtrip.duomec";
  std::filesystem::remove(path);
  auto created =
      ParametricDocument::create(std::make_unique<OcafDocumentStore>());
  if (!created) {
    std::cerr << created.error().message << '\n';
    return 1;
  }
  auto document = std::move(created).value();
  auto body = document->add_body("Main Body");
  auto feature =
      body
          ? document->add_generic_feature(body.value(), "Persistent Feature")
          : duomec::core::Result<duomec::cad::FeatureId>::failure(body.error());
  auto parameter =
      feature ? document->add_parameter(body.value(), feature.value(), "length",
                                        0.125)
              : duomec::core::Result<duomec::cad::ParameterId>::failure(
                    feature.error());
  if (!parameter || !document->save(path))
    return 2;
  const auto expected = document->snapshot();
  auto opened =
      ParametricDocument::open(std::make_unique<OcafDocumentStore>(), path);
  if (!opened || opened.value()->snapshot() != expected)
    return 3;
  if (!opened.value()->set_parameter(body.value(), feature.value(),
                                     parameter.value(), 0.250))
    return 4;
  if (!opened.value()->undo() || opened.value()->snapshot() != expected)
    return 5;
  if (!opened.value()->redo())
    return 6;
  std::filesystem::remove(path);
  return 0;
}

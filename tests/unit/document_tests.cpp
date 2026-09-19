#include "duomec/cad/document/memory_document_store.hpp"
#include "duomec/cad/document/parametric_document.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {
bool check(bool condition, const char *message) {
  if (!condition)
    std::cerr << "FAIL: " << message << '\n';
  return condition;
}
} // namespace

int main() {
  using namespace duomec::cad;
  bool ok = true;
  auto repository = std::make_shared<MemoryDocumentRepository>();
  auto created = ParametricDocument::create(
      std::make_unique<MemoryDocumentStore>(repository));
  ok &= check(created.has_value(), "create document");
  if (!created)
    return 1;
  auto document = std::move(created).value();
  const DocumentId document_id = document->snapshot().id;
  auto body = document->add_body("Body");
  ok &= check(body.has_value(), "add body");
  auto feature = document->add_generic_feature(body.value(), "Dummy feature");
  ok &= check(feature.has_value(), "add generic feature");

  ParameterId first_parameter = ParameterId::generate();
  for (int index = 0; index < 20; ++index) {
    auto parameter =
        document->add_parameter(body.value(), feature.value(),
                                "p" + std::to_string(index), index * 0.001);
    ok &= check(parameter.has_value(), "add parameter transaction");
    if (index == 0 && parameter)
      first_parameter = parameter.value();
  }
  ok &=
      check(document->snapshot().bodies[0].features[0].parameters.size() == 20,
            "twenty mutations committed");

  for (int index = 0; index < 20; ++index)
    ok &= check(document->undo().has_value(), "stress undo");
  ok &= check(document->snapshot().bodies[0].features[0].parameters.empty(),
              "twenty undos restore initial feature");
  for (int index = 0; index < 20; ++index)
    ok &= check(document->redo().has_value(), "stress redo");
  ok &=
      check(document->snapshot().bodies[0].features[0].parameters.size() == 20,
            "twenty redos restore parameters");

  auto failed = document->add_parameter(BodyId::generate(), feature.value(),
                                        "invalid", 1.0);
  ok &= check(!failed, "failed mutation aborts");
  ok &= check(document->undo().has_value(),
              "failed mutation creates no undo step");
  ok &=
      check(document->snapshot().bodies[0].features[0].parameters.size() == 19,
            "undo targets last successful mutation");
  ok &= check(document->redo().has_value(), "redo after aborted transaction");

  const std::filesystem::path path = "m1a_save_reload.duomec";
  const DocumentSnapshot before_save = document->snapshot();
  ok &= check(document->save(path).has_value(), "save document");
  auto opened = ParametricDocument::open(
      std::make_unique<MemoryDocumentStore>(repository), path);
  ok &= check(opened.has_value(), "reopen document");
  if (opened) {
    ok &= check(opened.value()->snapshot() == before_save,
                "IDs and parameters survive save/load");
    ok &= check(opened.value()->snapshot().id == document_id,
                "document UUID survives save/load");
    const double value =
        opened.value()->snapshot().bodies[0].features[0].parameters[0].value_si;
    ok &=
        check(std::abs(value) < 1e-15, "SI parameter value survives save/load");
  }
  ok &= check(!document->save("wrong-extension.cad").has_value(),
              "reject non-duomec extension");
  (void)first_parameter;
  return ok ? 0 : 1;
}

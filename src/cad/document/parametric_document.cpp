#include "duomec/cad/document/parametric_document.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace duomec::cad {
namespace {
core::Error not_found(std::string entity, std::string id) {
  return {core::ErrorCode::invalid_argument, std::move(entity) + " not found",
          std::move(id)};
}
} // namespace

ParametricDocument::ParametricDocument(std::unique_ptr<IDocumentStore> store,
                                       DocumentSnapshot snapshot)
    : store_(std::move(store)), snapshot_(std::move(snapshot)) {}

core::Result<std::unique_ptr<ParametricDocument>>
ParametricDocument::create(std::unique_ptr<IDocumentStore> store) {
  if (!store)
    return core::Result<std::unique_ptr<ParametricDocument>>::failure(
        {core::ErrorCode::invalid_argument, "document store is required", {}});
  DocumentSnapshot snapshot;
  auto initialized = store->initialize(snapshot);
  if (!initialized)
    return core::Result<std::unique_ptr<ParametricDocument>>::failure(
        initialized.error());
  return core::Result<std::unique_ptr<ParametricDocument>>::success(
      std::unique_ptr<ParametricDocument>(
          new ParametricDocument(std::move(store), std::move(snapshot))));
}

core::Result<std::unique_ptr<ParametricDocument>>
ParametricDocument::open(std::unique_ptr<IDocumentStore> store,
                         const std::filesystem::path &path) {
  if (!store)
    return core::Result<std::unique_ptr<ParametricDocument>>::failure(
        {core::ErrorCode::invalid_argument, "document store is required", {}});
  auto loaded = store->load(path);
  if (!loaded)
    return core::Result<std::unique_ptr<ParametricDocument>>::failure(
        loaded.error());
  auto snapshot = std::move(loaded).value();
  return core::Result<std::unique_ptr<ParametricDocument>>::success(
      std::unique_ptr<ParametricDocument>(
          new ParametricDocument(std::move(store), std::move(snapshot))));
}

core::Result<bool> ParametricDocument::mutate(
    const std::function<core::Result<bool>(DocumentSnapshot &)> &operation) {
  DocumentSnapshot candidate = snapshot_;
  auto begun = store_->begin_transaction();
  if (!begun)
    return begun;
  auto changed = operation(candidate);
  if (!changed) {
    store_->abort_transaction();
    return changed;
  }
  auto written = store_->write(candidate);
  if (!written) {
    store_->abort_transaction();
    return written;
  }
  auto committed = store_->commit_transaction();
  if (!committed) {
    store_->abort_transaction();
    return committed;
  }
  snapshot_ = std::move(candidate);
  return core::Result<bool>::success(true);
}

core::Result<BodyId> ParametricDocument::add_body(std::string name) {
  if (name.empty())
    return core::Result<BodyId>::failure(
        {core::ErrorCode::invalid_argument, "body name must not be empty", {}});
  Body body{BodyId::generate(), std::move(name), {}};
  const BodyId id = body.id;
  auto result = mutate([&](DocumentSnapshot &snapshot) {
    snapshot.bodies.push_back(std::move(body));
    return core::Result<bool>::success(true);
  });
  if (!result)
    return core::Result<BodyId>::failure(result.error());
  return core::Result<BodyId>::success(id);
}

core::Result<FeatureId>
ParametricDocument::add_generic_feature(BodyId body_id, std::string name) {
  if (name.empty())
    return core::Result<FeatureId>::failure({core::ErrorCode::invalid_argument,
                                             "feature name must not be empty",
                                             {}});
  Feature feature{FeatureId::generate(),
                  std::move(name),
                  FeatureType::generic,
                  true,
                  {},
                  {},
                  FeatureExecutionStatus::not_executed,
                  RecomputeState::clean,
                  {}};
  const FeatureId id = feature.id;
  auto result = mutate([&](DocumentSnapshot &snapshot) {
    auto body = std::ranges::find(snapshot.bodies, body_id, &Body::id);
    if (body == snapshot.bodies.end())
      return core::Result<bool>::failure(not_found("body", body_id.value()));
    body->features.push_back(std::move(feature));
    return core::Result<bool>::success(true);
  });
  if (!result)
    return core::Result<FeatureId>::failure(result.error());
  return core::Result<FeatureId>::success(id);
}

core::Result<ParameterId>
ParametricDocument::add_parameter(BodyId body_id, FeatureId feature_id,
                                  std::string name, double value_si) {
  if (name.empty() || !std::isfinite(value_si))
    return core::Result<ParameterId>::failure(
        {core::ErrorCode::invalid_argument,
         "parameter requires a name and finite SI value",
         {}});
  Parameter parameter{ParameterId::generate(), std::move(name), value_si};
  const ParameterId id = parameter.id;
  auto result = mutate([&](DocumentSnapshot &snapshot) {
    auto body = std::ranges::find(snapshot.bodies, body_id, &Body::id);
    if (body == snapshot.bodies.end())
      return core::Result<bool>::failure(not_found("body", body_id.value()));
    auto feature = std::ranges::find(body->features, feature_id, &Feature::id);
    if (feature == body->features.end())
      return core::Result<bool>::failure(
          not_found("feature", feature_id.value()));
    feature->parameters.push_back(std::move(parameter));
    return core::Result<bool>::success(true);
  });
  if (!result)
    return core::Result<ParameterId>::failure(result.error());
  return core::Result<ParameterId>::success(id);
}

core::Result<bool> ParametricDocument::set_parameter(BodyId body_id,
                                                     FeatureId feature_id,
                                                     ParameterId parameter_id,
                                                     double value_si) {
  if (!std::isfinite(value_si))
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "parameter value must be finite",
                                        parameter_id.value()});
  return mutate([&](DocumentSnapshot &snapshot) {
    auto body = std::ranges::find(snapshot.bodies, body_id, &Body::id);
    if (body == snapshot.bodies.end())
      return core::Result<bool>::failure(not_found("body", body_id.value()));
    auto feature = std::ranges::find(body->features, feature_id, &Feature::id);
    if (feature == body->features.end())
      return core::Result<bool>::failure(
          not_found("feature", feature_id.value()));
    auto parameter =
        std::ranges::find(feature->parameters, parameter_id, &Parameter::id);
    if (parameter == feature->parameters.end())
      return core::Result<bool>::failure(
          not_found("parameter", parameter_id.value()));
    parameter->value_si = value_si;
    feature->recompute_state = RecomputeState::dirty;
    return core::Result<bool>::success(true);
  });
}

core::Result<bool> ParametricDocument::undo() {
  auto restored = store_->undo();
  if (!restored)
    return core::Result<bool>::failure(restored.error());
  snapshot_ = std::move(restored).value();
  return core::Result<bool>::success(true);
}
core::Result<bool> ParametricDocument::redo() {
  auto restored = store_->redo();
  if (!restored)
    return core::Result<bool>::failure(restored.error());
  snapshot_ = std::move(restored).value();
  return core::Result<bool>::success(true);
}
core::Result<bool> ParametricDocument::save(const std::filesystem::path &path) {
  if (path.extension() != ".duomec")
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "document path must use .duomec",
                                        path.string()});
  return store_->save_atomic(path);
}

} // namespace duomec::cad

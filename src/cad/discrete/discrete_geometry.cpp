#include "duomec/cad/discrete/discrete_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace duomec::cad::discrete {
namespace {
core::Error history_error(std::string message, std::string context) {
  return {core::ErrorCode::invalid_argument, std::move(message),
          std::move(context)};
}
} // namespace

ReverseEngineeringTolerancePolicy ReverseEngineeringTolerancePolicy::preset(
    ReverseEngineeringTolerancePreset preset_value) {
  switch (preset_value) {
  case ReverseEngineeringTolerancePreset::precision_machined_scan:
    return {5.0e-6, 5.0e-6, 1.0e-5, 1.0e-5, 2.0e-5, 1.0e-5, 2.5e-5};
  case ReverseEngineeringTolerancePreset::general_mechanical_scan:
    return {5.0e-5, 5.0e-5, 1.0e-4, 1.0e-4, 2.0e-4, 1.0e-4, 2.5e-4};
  case ReverseEngineeringTolerancePreset::coarse_scan:
    return {5.0e-4, 5.0e-4, 1.0e-3, 1.0e-3, 2.0e-3, 1.0e-3, 2.5e-3};
  case ReverseEngineeringTolerancePreset::custom:
    return {};
  }
  return {};
}

core::Result<bool> ReverseEngineeringTolerancePolicy::validate() const {
  const double values[] = {source_noise_si,
                           merge_tolerance_si,
                           primitive_fit_tolerance_si,
                           curve_fit_tolerance_si,
                           surface_fit_tolerance_si,
                           sewing_tolerance_si,
                           reconstruction_validation_tolerance_si};
  if (!std::ranges::all_of(values, [](double value) {
        return std::isfinite(value) && value > 0.0;
      }))
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument,
         "reverse-engineering tolerances must be finite positive SI values",
         {}});
  return core::Result<bool>::success(true);
}

MeshFeature::MeshFeature(MeshOperation operation,
                         ProcessingFeatureRecord record)
    : operation_(operation), record_(std::move(record)) {}
MeshOperation MeshFeature::operation() const noexcept { return operation_; }
const ProcessingFeatureRecord &MeshFeature::record() const noexcept {
  return record_;
}
PointCloudFeature::PointCloudFeature(PointCloudOperation operation,
                                     ProcessingFeatureRecord record)
    : operation_(operation), record_(std::move(record)) {}
PointCloudOperation PointCloudFeature::operation() const noexcept {
  return operation_;
}
const ProcessingFeatureRecord &PointCloudFeature::record() const noexcept {
  return record_;
}

MeshProcessingPipeline::MeshProcessingPipeline(
    std::vector<MeshFeature> features)
    : features_(std::move(features)) {}
core::Result<DiscreteFeatureId> MeshProcessingPipeline::append(
    MeshOperation operation, std::string name,
    std::map<std::string, ProcessingParameter, std::less<>> parameters) {
  if (name.empty())
    return core::Result<DiscreteFeatureId>::failure(
        history_error("mesh feature name must not be empty", {}));
  ProcessingFeatureRecord record;
  record.name = std::move(name);
  record.parameters = std::move(parameters);
  if (!features_.empty() && features_.back().record_.output_revision)
    record.input_revision = features_.back().record_.output_revision;
  const DiscreteFeatureId id = record.id;
  features_.emplace_back(operation, std::move(record));
  return core::Result<DiscreteFeatureId>::success(id);
}
void MeshProcessingPipeline::invalidate_from(std::size_t index) {
  for (; index < features_.size(); ++index) {
    auto &record = features_[index].record_;
    record.output_revision.reset();
    record.cache_key.clear();
    record.input_revision.reset();
    record.state =
        record.enabled ? ProcessingState::dirty : ProcessingState::suppressed;
  }
}
core::Result<bool> MeshProcessingPipeline::set_parameter(
    DiscreteFeatureId feature, std::string key, ProcessingParameter value) {
  if (key.empty())
    return core::Result<bool>::failure(
        history_error("processing parameter key must not be empty", {}));
  const auto item =
      std::ranges::find(features_, feature, [](const MeshFeature &entry) {
        return entry.record_.id;
      });
  if (item == features_.end())
    return core::Result<bool>::failure(
        history_error("mesh feature not found", feature.value()));
  item->record_.parameters.insert_or_assign(std::move(key), std::move(value));
  invalidate_from(static_cast<std::size_t>(item - features_.begin()));
  return core::Result<bool>::success(true);
}
core::Result<bool>
MeshProcessingPipeline::set_suppressed(DiscreteFeatureId feature,
                                       bool suppressed) {
  const auto item =
      std::ranges::find(features_, feature, [](const MeshFeature &entry) {
        return entry.record_.id;
      });
  if (item == features_.end())
    return core::Result<bool>::failure(
        history_error("mesh feature not found", feature.value()));
  item->record_.enabled = !suppressed;
  invalidate_from(static_cast<std::size_t>(item - features_.begin()));
  return core::Result<bool>::success(true);
}
core::Result<bool>
MeshProcessingPipeline::accept_result(DiscreteFeatureId feature,
                                      DiscreteGeometryRevision output,
                                      std::string cache_key) {
  const auto item =
      std::ranges::find(features_, feature, [](const MeshFeature &entry) {
        return entry.record_.id;
      });
  if (item == features_.end() || !item->record_.enabled)
    return core::Result<bool>::failure(
        history_error("enabled mesh feature not found", feature.value()));
  if (output.content_hash.empty() ||
      (item->record_.cacheable && cache_key.empty()))
    return core::Result<bool>::failure(history_error(
        "mesh result requires revision hash and cache key", feature.value()));
  const auto index = static_cast<std::size_t>(item - features_.begin());
  if (index > 0) {
    const auto &previous = features_[index - 1].record_;
    if (previous.enabled && previous.state != ProcessingState::clean)
      return core::Result<bool>::failure(
          history_error("upstream mesh feature is not clean", feature.value()));
    item->record_.input_revision = previous.output_revision;
  }
  item->record_.output_revision = std::move(output);
  item->record_.cache_key = std::move(cache_key);
  item->record_.state = ProcessingState::clean;
  if (index + 1 < features_.size())
    invalidate_from(index + 1);
  return core::Result<bool>::success(true);
}
const std::vector<MeshFeature> &
MeshProcessingPipeline::features() const noexcept {
  return features_;
}

PointCloudProcessingPipeline::PointCloudProcessingPipeline(
    std::vector<PointCloudFeature> features)
    : features_(std::move(features)) {}
core::Result<DiscreteFeatureId> PointCloudProcessingPipeline::append(
    PointCloudOperation operation, std::string name,
    std::map<std::string, ProcessingParameter, std::less<>> parameters) {
  if (name.empty())
    return core::Result<DiscreteFeatureId>::failure(
        history_error("point-cloud feature name must not be empty", {}));
  ProcessingFeatureRecord record;
  record.name = std::move(name);
  record.parameters = std::move(parameters);
  if (!features_.empty() && features_.back().record_.output_revision)
    record.input_revision = features_.back().record_.output_revision;
  const DiscreteFeatureId id = record.id;
  features_.emplace_back(operation, std::move(record));
  return core::Result<DiscreteFeatureId>::success(id);
}
void PointCloudProcessingPipeline::invalidate_from(std::size_t index) {
  for (; index < features_.size(); ++index) {
    auto &record = features_[index].record_;
    record.output_revision.reset();
    record.cache_key.clear();
    record.input_revision.reset();
    record.state =
        record.enabled ? ProcessingState::dirty : ProcessingState::suppressed;
  }
}
core::Result<bool> PointCloudProcessingPipeline::set_parameter(
    DiscreteFeatureId feature, std::string key, ProcessingParameter value) {
  if (key.empty())
    return core::Result<bool>::failure(
        history_error("processing parameter key must not be empty", {}));
  const auto item =
      std::ranges::find(features_, feature, [](const PointCloudFeature &entry) {
        return entry.record_.id;
      });
  if (item == features_.end())
    return core::Result<bool>::failure(
        history_error("point-cloud feature not found", feature.value()));
  item->record_.parameters.insert_or_assign(std::move(key), std::move(value));
  invalidate_from(static_cast<std::size_t>(item - features_.begin()));
  return core::Result<bool>::success(true);
}
core::Result<bool>
PointCloudProcessingPipeline::set_suppressed(DiscreteFeatureId feature,
                                             bool suppressed) {
  const auto item =
      std::ranges::find(features_, feature, [](const PointCloudFeature &entry) {
        return entry.record_.id;
      });
  if (item == features_.end())
    return core::Result<bool>::failure(
        history_error("point-cloud feature not found", feature.value()));
  item->record_.enabled = !suppressed;
  invalidate_from(static_cast<std::size_t>(item - features_.begin()));
  return core::Result<bool>::success(true);
}
core::Result<bool>
PointCloudProcessingPipeline::accept_result(DiscreteFeatureId feature,
                                            DiscreteGeometryRevision output,
                                            std::string cache_key) {
  const auto item =
      std::ranges::find(features_, feature, [](const PointCloudFeature &entry) {
        return entry.record_.id;
      });
  if (item == features_.end() || !item->record_.enabled)
    return core::Result<bool>::failure(history_error(
        "enabled point-cloud feature not found", feature.value()));
  if (output.content_hash.empty() ||
      (item->record_.cacheable && cache_key.empty()))
    return core::Result<bool>::failure(
        history_error("point-cloud result requires revision hash and cache key",
                      feature.value()));
  const auto index = static_cast<std::size_t>(item - features_.begin());
  if (index > 0) {
    const auto &previous = features_[index - 1].record_;
    if (previous.enabled && previous.state != ProcessingState::clean)
      return core::Result<bool>::failure(history_error(
          "upstream point-cloud feature is not clean", feature.value()));
    item->record_.input_revision = previous.output_revision;
  }
  item->record_.output_revision = std::move(output);
  item->record_.cache_key = std::move(cache_key);
  item->record_.state = ProcessingState::clean;
  if (index + 1 < features_.size())
    invalidate_from(index + 1);
  return core::Result<bool>::success(true);
}
const std::vector<PointCloudFeature> &
PointCloudProcessingPipeline::features() const noexcept {
  return features_;
}

MeshBody::MeshBody(MeshBodyId id, std::string name, MeshSnapshot source,
                   MeshProcessingPipeline history)
    : id_(std::move(id)), name_(std::move(name)), source_(std::move(source)),
      history_(std::move(history)) {}
MeshBodyId MeshBody::id() const { return id_; }
const std::string &MeshBody::name() const noexcept { return name_; }
const MeshSnapshot &MeshBody::source() const noexcept { return source_; }
MeshProcessingPipeline &MeshBody::history() noexcept { return history_; }
const MeshProcessingPipeline &MeshBody::history() const noexcept {
  return history_;
}
PointCloudBody::PointCloudBody(PointCloudBodyId id, std::string name,
                               PointCloudSnapshot source,
                               PointCloudProcessingPipeline history)
    : id_(std::move(id)), name_(std::move(name)), source_(std::move(source)),
      history_(std::move(history)) {}
PointCloudBodyId PointCloudBody::id() const { return id_; }
const std::string &PointCloudBody::name() const noexcept { return name_; }
const PointCloudSnapshot &PointCloudBody::source() const noexcept {
  return source_;
}
PointCloudProcessingPipeline &PointCloudBody::history() noexcept {
  return history_;
}
const PointCloudProcessingPipeline &PointCloudBody::history() const noexcept {
  return history_;
}

} // namespace duomec::cad::discrete

#include "duomec/cad/features/feature_framework.hpp"

#include <chrono>
#include <cmath>
#include <utility>

namespace duomec::cad::features {
namespace {
FeatureDiagnostic diagnostic(FeatureDiagnosticCode code, std::string message,
                             std::string subject) {
  return {code,
          FeatureDiagnosticSeverity::error,
          std::move(message),
          std::move(subject),
          {}};
}

bool value_matches(ParameterType type, const ParameterValue &value) {
  switch (type) {
  case ParameterType::boolean:
    return std::holds_alternative<bool>(value);
  case ParameterType::integer:
    return std::holds_alternative<std::int64_t>(value);
  case ParameterType::scalar:
    return std::holds_alternative<double>(value);
  case ParameterType::text:
    return std::holds_alternative<std::string>(value);
  case ParameterType::direction:
    return std::holds_alternative<Direction3d>(value);
  }
  return false;
}

bool valid_direction(const Direction3d &direction) {
  const double squared = direction.x * direction.x + direction.y * direction.y +
                         direction.z * direction.z;
  return std::isfinite(squared) && squared > 1.0e-24;
}

FeatureResult invalid_result(std::vector<FeatureDiagnostic> diagnostics) {
  FeatureResult result;
  result.status = FeatureStatus::failure;
  result.diagnostics = std::move(diagnostics);
  return result;
}
} // namespace

const ParameterDefinition *
FeatureParameterSchema::find(std::string_view key) const {
  const auto item =
      std::ranges::find(parameters, key, &ParameterDefinition::key);
  return item == parameters.end() ? nullptr : &*item;
}

SelectionCollector::SelectionCollector(SelectionCollectorDefinition definition)
    : definition_(std::move(definition)) {}

const SelectionCollectorDefinition &
SelectionCollector::definition() const noexcept {
  return definition_;
}

const std::vector<SelectionReference> &
SelectionCollector::selections() const noexcept {
  return selections_;
}

core::Result<bool> SelectionCollector::add(SelectionReference selection) {
  if (!definition_.rule.allowed_kinds.contains(selection.kind)) {
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument,
         "selection kind is not accepted by collector", definition_.key});
  }
  if (selection.persistent_reference.empty()) {
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument,
         "selection requires a persistent reference", definition_.key});
  }
  const auto duplicate =
      std::ranges::find(selections_, selection.persistent_reference,
                        &SelectionReference::persistent_reference);
  if (!definition_.rule.allow_duplicates && duplicate != selections_.end()) {
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "duplicate selection",
                                        selection.persistent_reference});
  }
  if (definition_.rule.maximum_count &&
      selections_.size() >= *definition_.rule.maximum_count) {
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "selection collector is full",
                                        definition_.key});
  }
  selections_.push_back(std::move(selection));
  return core::Result<bool>::success(true);
}

bool SelectionCollector::remove(std::string_view persistent_reference) {
  const auto old_size = selections_.size();
  std::erase_if(selections_, [&](const SelectionReference &selection) {
    return selection.persistent_reference == persistent_reference;
  });
  return selections_.size() != old_size;
}

std::size_t SelectionCollector::consume_preselection(
    const std::vector<SelectionReference> &preselection) {
  if (!definition_.consume_preselection)
    return 0;
  std::size_t consumed = 0;
  for (const auto &selection : preselection) {
    if (add(selection))
      ++consumed;
  }
  return consumed;
}

std::vector<FeatureDiagnostic> SelectionCollector::validate() const {
  std::vector<FeatureDiagnostic> diagnostics;
  if (selections_.size() < definition_.rule.minimum_count) {
    diagnostics.push_back(diagnostic(
        FeatureDiagnosticCode::missing_selection,
        "collector requires at least " +
            std::to_string(definition_.rule.minimum_count) + " selection(s)",
        definition_.key));
  }
  for (const auto &selection : selections_) {
    if (!selection.resolved) {
      diagnostics.push_back(diagnostic(FeatureDiagnosticCode::invalid_selection,
                                       "selected reference is unresolved",
                                       selection.persistent_reference));
    }
  }
  return diagnostics;
}

FeaturePreviewSession::FeaturePreviewSession(
    FeatureDefinition definition, IFeatureExecutor &executor,
    IFeatureTransaction &transaction, std::optional<FeatureId> edited_feature)
    : definition_(std::move(definition)), executor_(executor),
      transaction_(transaction) {
  context_.edited_feature = std::move(edited_feature);
  for (const auto &parameter : definition_.parameter_schema.parameters)
    context_.parameters.emplace(parameter.key, parameter.default_value);
  for (const auto &collector : definition_.collectors)
    collectors_.emplace_back(collector);
  if (!definition_.supported_operations.empty())
    context_.body_operation = *definition_.supported_operations.begin();
  synchronize_selections();
}

const FeatureDefinition &FeaturePreviewSession::definition() const noexcept {
  return definition_;
}
const FeatureExecutionContext &FeaturePreviewSession::context() const noexcept {
  return context_;
}
PreviewSessionState FeaturePreviewSession::state() const noexcept {
  return state_;
}
const std::optional<FeatureResult> &
FeaturePreviewSession::latest_preview() const noexcept {
  return latest_preview_;
}

bool FeaturePreviewSession::terminal() const noexcept {
  return state_ == PreviewSessionState::committed ||
         state_ == PreviewSessionState::cancelled;
}

core::Result<bool> FeaturePreviewSession::set_parameter(std::string_view key,
                                                        ParameterValue value) {
  if (terminal())
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument, "preview session is closed", {}});
  const auto *definition = definition_.parameter_schema.find(key);
  if (!definition)
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "unknown feature parameter",
                                        std::string(key)});
  if (!value_matches(definition->type, value))
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "feature parameter type mismatch",
                                        std::string(key)});
  context_.parameters.insert_or_assign(std::string(key), std::move(value));
  state_ = PreviewSessionState::editing;
  return core::Result<bool>::success(true);
}

core::Result<bool>
FeaturePreviewSession::set_body_operation(BodyOperationMode operation) {
  if (terminal() || !definition_.supported_operations.contains(operation))
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "unsupported body operation",
                                        definition_.type_key});
  context_.body_operation = operation;
  state_ = PreviewSessionState::editing;
  return core::Result<bool>::success(true);
}

void FeaturePreviewSession::set_extent(ExtentDefinition extent) {
  if (!terminal()) {
    context_.extent = std::move(extent);
    state_ = PreviewSessionState::editing;
  }
}
void FeaturePreviewSession::set_direction(DirectionDefinition direction) {
  if (!terminal()) {
    context_.direction = direction;
    state_ = PreviewSessionState::editing;
  }
}

core::Result<bool>
FeaturePreviewSession::add_selection(std::string_view collector_key,
                                     SelectionReference selection) {
  if (terminal())
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument, "preview session is closed", {}});
  const auto collector = std::ranges::find(
      collectors_, collector_key,
      [](const SelectionCollector &item) { return item.definition().key; });
  if (collector == collectors_.end())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "unknown selection collector",
                                        std::string(collector_key)});
  auto added = collector->add(std::move(selection));
  if (added) {
    synchronize_selections();
    state_ = PreviewSessionState::editing;
  }
  return added;
}

core::Result<bool>
FeaturePreviewSession::remove_selection(std::string_view collector_key,
                                        std::string_view persistent_reference) {
  if (terminal())
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument, "preview session is closed", {}});
  const auto collector = std::ranges::find(
      collectors_, collector_key,
      [](const SelectionCollector &item) { return item.definition().key; });
  if (collector == collectors_.end())
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "unknown selection collector",
                                        std::string(collector_key)});
  if (!collector->remove(persistent_reference))
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "selection is not in collector",
                                        std::string(persistent_reference)});
  synchronize_selections();
  state_ = PreviewSessionState::editing;
  return core::Result<bool>::success(true);
}

std::size_t FeaturePreviewSession::consume_preselection(
    const std::vector<SelectionReference> &preselection) {
  if (terminal())
    return 0;
  std::size_t consumed = 0;
  for (const auto &selection : preselection) {
    for (auto &collector : collectors_) {
      if (!collector.definition().consume_preselection)
        continue;
      if (collector.add(selection)) {
        ++consumed;
        break;
      }
    }
  }
  synchronize_selections();
  if (consumed > 0)
    state_ = PreviewSessionState::editing;
  return consumed;
}

void FeaturePreviewSession::synchronize_selections() {
  context_.selections.clear();
  for (const auto &collector : collectors_)
    context_.selections.emplace(collector.definition().key,
                                collector.selections());
}

std::vector<FeatureDiagnostic> FeaturePreviewSession::validate() const {
  std::vector<FeatureDiagnostic> diagnostics;
  if (definition_.type_key.empty())
    diagnostics.push_back(diagnostic(FeatureDiagnosticCode::invalid_parameter,
                                     "feature type key is empty", "type_key"));
  std::set<std::string, std::less<>> keys;
  for (const auto &parameter : definition_.parameter_schema.parameters) {
    if (parameter.key.empty() || !keys.insert(parameter.key).second) {
      diagnostics.push_back(diagnostic(FeatureDiagnosticCode::invalid_parameter,
                                       "parameter key is empty or duplicated",
                                       parameter.key));
      continue;
    }
    const auto value = context_.parameters.find(parameter.key);
    if (value == context_.parameters.end()) {
      if (parameter.required)
        diagnostics.push_back(
            diagnostic(FeatureDiagnosticCode::invalid_parameter,
                       "required feature parameter is missing", parameter.key));
      continue;
    }
    if (!value_matches(parameter.type, value->second)) {
      diagnostics.push_back(diagnostic(FeatureDiagnosticCode::invalid_parameter,
                                       "feature parameter has the wrong type",
                                       parameter.key));
      continue;
    }
    if (const auto scalar = std::get_if<double>(&value->second)) {
      if (!std::isfinite(*scalar) ||
          (parameter.minimum && *scalar < *parameter.minimum) ||
          (parameter.maximum && *scalar > *parameter.maximum))
        diagnostics.push_back(diagnostic(
            FeatureDiagnosticCode::invalid_parameter,
            "scalar parameter is outside its valid range", parameter.key));
    }
    if (const auto integer = std::get_if<std::int64_t>(&value->second)) {
      const double numeric = static_cast<double>(*integer);
      if ((parameter.minimum && numeric < *parameter.minimum) ||
          (parameter.maximum && numeric > *parameter.maximum))
        diagnostics.push_back(diagnostic(
            FeatureDiagnosticCode::invalid_parameter,
            "integer parameter is outside its valid range", parameter.key));
    }
    if (const auto direction = std::get_if<Direction3d>(&value->second);
        direction && !valid_direction(*direction))
      diagnostics.push_back(diagnostic(
          FeatureDiagnosticCode::invalid_parameter,
          "direction parameter must be finite and non-zero", parameter.key));
  }
  if (!definition_.supported_operations.contains(context_.body_operation))
    diagnostics.push_back(
        diagnostic(FeatureDiagnosticCode::unsupported_operation,
                   "body operation is not supported by this feature",
                   definition_.type_key));
  for (const auto &collector : collectors_) {
    auto collector_diagnostics = collector.validate();
    diagnostics.insert(diagnostics.end(), collector_diagnostics.begin(),
                       collector_diagnostics.end());
  }
  std::set<std::string, std::less<>> collector_keys;
  for (const auto &collector : definition_.collectors) {
    if (collector.key.empty() || !collector_keys.insert(collector.key).second)
      diagnostics.push_back(diagnostic(FeatureDiagnosticCode::invalid_parameter,
                                       "collector key is empty or duplicated",
                                       collector.key));
    if (collector.rule.allowed_kinds.empty() ||
        (collector.rule.maximum_count &&
         *collector.rule.maximum_count < collector.rule.minimum_count))
      diagnostics.push_back(diagnostic(
          FeatureDiagnosticCode::invalid_parameter,
          "collector rule has an invalid kind or cardinality", collector.key));
  }
  for (const auto &group : definition_.panel_groups) {
    for (const auto &parameter_key : group.parameter_keys)
      if (!definition_.parameter_schema.find(parameter_key))
        diagnostics.push_back(
            diagnostic(FeatureDiagnosticCode::invalid_parameter,
                       "panel references an unknown parameter", parameter_key));
    for (const auto &collector_key : group.collector_keys)
      if (!collector_keys.contains(collector_key))
        diagnostics.push_back(
            diagnostic(FeatureDiagnosticCode::invalid_parameter,
                       "panel references an unknown collector", collector_key));
  }
  for (const auto &manipulator : definition_.manipulators)
    if (!definition_.parameter_schema.find(manipulator.parameter_key))
      diagnostics.push_back(
          diagnostic(FeatureDiagnosticCode::invalid_parameter,
                     "manipulator references an unknown parameter",
                     manipulator.parameter_key));
  if (context_.extent &&
      (context_.extent->kind == ExtentKind::blind ||
       context_.extent->kind == ExtentKind::symmetric) &&
      (!std::isfinite(context_.extent->distance_si) ||
       context_.extent->distance_si <= 0.0))
    diagnostics.push_back(
        diagnostic(FeatureDiagnosticCode::invalid_parameter,
                   "extent distance must be positive SI length", "extent"));
  if (context_.direction && !valid_direction(context_.direction->vector))
    diagnostics.push_back(diagnostic(FeatureDiagnosticCode::invalid_parameter,
                                     "execution direction is invalid",
                                     "direction"));
  return diagnostics;
}

core::Result<FeatureResult> FeaturePreviewSession::preview() {
  if (terminal())
    return core::Result<FeatureResult>::failure(
        {core::ErrorCode::invalid_argument, "preview session is closed", {}});
  auto diagnostics = validate();
  if (!diagnostics.empty()) {
    state_ = PreviewSessionState::invalid;
    latest_preview_ = invalid_result(std::move(diagnostics));
    return core::Result<FeatureResult>::success(*latest_preview_);
  }
  request_preview_cancel();
  cancellation_ = std::stop_source{};
  context_.quality = ExecutionQuality::preview;
  const auto started = std::chrono::steady_clock::now();
  FeatureResult result =
      executor_.execute(definition_, context_, cancellation_.get_token());
  result.execution_time = std::chrono::steady_clock::now() - started;
  latest_preview_ = result;
  state_ = result.status == FeatureStatus::success ||
                   result.status == FeatureStatus::warning
               ? PreviewSessionState::preview_ready
               : PreviewSessionState::invalid;
  return core::Result<FeatureResult>::success(std::move(result));
}

core::Result<FeatureResult> FeaturePreviewSession::commit() {
  if (terminal())
    return core::Result<FeatureResult>::failure(
        {core::ErrorCode::invalid_argument, "preview session is closed", {}});
  auto diagnostics = validate();
  if (!diagnostics.empty()) {
    state_ = PreviewSessionState::invalid;
    return core::Result<FeatureResult>::success(
        invalid_result(std::move(diagnostics)));
  }
  request_preview_cancel();
  cancellation_ = std::stop_source{};
  FeatureExecutionContext final_context = context_;
  final_context.quality = ExecutionQuality::final;
  const auto started = std::chrono::steady_clock::now();
  auto result = transaction_.commit(definition_, final_context, executor_,
                                    cancellation_.get_token());
  if (!result)
    return result;
  FeatureResult committed_result = std::move(result).value();
  committed_result.execution_time = std::chrono::steady_clock::now() - started;
  if (committed_result.status == FeatureStatus::success ||
      committed_result.status == FeatureStatus::warning) {
    context_ = std::move(final_context);
    state_ = PreviewSessionState::committed;
  } else {
    state_ = PreviewSessionState::invalid;
  }
  return core::Result<FeatureResult>::success(std::move(committed_result));
}

void FeaturePreviewSession::request_preview_cancel() noexcept {
  cancellation_.request_stop();
}
void FeaturePreviewSession::cancel() noexcept {
  if (terminal())
    return;
  request_preview_cancel();
  latest_preview_.reset();
  state_ = PreviewSessionState::cancelled;
}

} // namespace duomec::cad::features

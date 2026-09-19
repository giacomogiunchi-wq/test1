#pragma once

#include "duomec/cad/document/ids.hpp"

#include <string>
#include <vector>

namespace duomec::cad {

enum class FeatureType { generic };
enum class FeatureExecutionStatus { not_executed, succeeded, failed };
enum class RecomputeState { clean, dirty, failed, blocked };
enum class DiagnosticSeverity { information, warning, error };
enum class DiagnosticCode {
  feature_invalid_input,
  document_save_failed,
  document_load_failed,
  dependency_cycle,
  upstream_feature_failed
};

struct Diagnostic {
  DiagnosticCode code{};
  DiagnosticSeverity severity{};
  std::string message;
  std::string owner_id;
  std::string technical_details;
  auto operator<=>(const Diagnostic &) const = default;
};

struct Parameter {
  ParameterId id{ParameterId::generate()};
  std::string name;
  double value_si{};
  auto operator<=>(const Parameter &) const = default;
};

struct Feature {
  FeatureId id{FeatureId::generate()};
  std::string name;
  FeatureType type{FeatureType::generic};
  bool enabled{true};
  std::vector<FeatureId> inputs;
  std::vector<Parameter> parameters;
  FeatureExecutionStatus execution_status{FeatureExecutionStatus::not_executed};
  RecomputeState recompute_state{RecomputeState::clean};
  std::vector<Diagnostic> diagnostics;
  auto operator<=>(const Feature &) const = default;
};

struct Body {
  BodyId id{BodyId::generate()};
  std::string name;
  std::vector<Feature> features;
  auto operator<=>(const Body &) const = default;
};

struct DocumentSnapshot {
  static constexpr int current_format_version = 1;
  int format_version{current_format_version};
  DocumentId id{DocumentId::generate()};
  std::vector<Body> bodies;
  auto operator<=>(const DocumentSnapshot &) const = default;
};

} // namespace duomec::cad

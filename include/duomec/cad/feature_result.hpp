#pragma once

#include "duomec/cad/document/ids.hpp"
#include "duomec/interfaces/interfaces.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace duomec::cad {

enum class FeatureStatus { success, warning, failure, cancelled };
enum class FeatureDiagnosticSeverity { information, warning, error };
enum class FeatureDiagnosticCode {
  invalid_parameter,
  missing_selection,
  invalid_selection,
  unsupported_operation,
  preview_cancelled,
  execution_failed,
  invalid_result
};

struct FeatureDiagnostic {
  FeatureDiagnosticCode code{FeatureDiagnosticCode::execution_failed};
  FeatureDiagnosticSeverity severity{FeatureDiagnosticSeverity::error};
  std::string message;
  std::string subject;
  std::string technical_details;
  auto operator<=>(const FeatureDiagnostic &) const = default;
};

struct TopologyChange {
  EntityId source;
  std::vector<EntityId> generated;
  std::vector<EntityId> modified;
  bool deleted{};
  auto operator<=>(const TopologyChange &) const = default;
};

// Domain-only reference intent. Resolution data is deliberately opaque here;
// the future OCCT/TNaming adapter interprets the historical token and derives
// signatures without exposing kernel topology in this public header.
struct TopologicalReference {
  TopologyReferenceId id{TopologyReferenceId::generate()};
  FeatureId producer{FeatureId::generate()};
  std::string expected_topology_kind;
  std::string historical_token;
  std::string geometric_signature;
  std::vector<std::string> adjacency_context;
  std::optional<std::string> user_intent;
};

struct FeatureResult {
  FeatureStatus status{FeatureStatus::failure};
  std::vector<GeometryShape> shapes;
  std::vector<BodyId> resulting_bodies;
  std::vector<FeatureDiagnostic> diagnostics;
  std::vector<TopologyChange> topology_evolution;
  std::chrono::nanoseconds execution_time{};
};

} // namespace duomec::cad

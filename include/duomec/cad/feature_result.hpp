#pragma once
#include "duomec/interfaces/interfaces.hpp"
#include <string>
#include <vector>
namespace duomec::cad {
enum class FeatureStatus { success, warning, failure };
struct TopologyChange { EntityId source; std::vector<EntityId> generated; std::vector<EntityId> modified; bool deleted{}; };
struct FeatureResult { FeatureStatus status{FeatureStatus::failure}; GeometryShape shape; std::vector<std::string> warnings; std::vector<std::string> diagnostics; std::vector<TopologyChange> topology_map; };
struct TopologicalReference { std::string naming_path; std::string geometric_signature; std::vector<std::string> adjacency_context; bool allow_fallback_reidentification{true}; };
}

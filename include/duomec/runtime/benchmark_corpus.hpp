#pragma once
#include "duomec/core/error.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace duomec::runtime {
struct LargeAssemblyScenario {
  std::string id;
  std::uint64_t seed{};
  std::uint64_t uniqueDefinitions{};
  std::uint64_t occurrences{};
  std::uint32_t maximumDepth{};
  std::uint32_t openTabs{};
  std::uint64_t estimatedGeometryBytes{};
  std::uint32_t networkLatencyMilliseconds{};
  std::uint32_t networkBandwidthMegabits{};
  auto operator<=>(const LargeAssemblyScenario &) const = default;
};

[[nodiscard]] std::vector<LargeAssemblyScenario> makeLargeAssemblyCorpus();
[[nodiscard]] std::string encodeCorpus(const std::vector<LargeAssemblyScenario> &corpus);
[[nodiscard]] core::Result<std::vector<LargeAssemblyScenario>> decodeCorpus(const std::string &text);
} // namespace duomec::runtime

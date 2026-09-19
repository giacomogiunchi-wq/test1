#include "duomec/runtime/benchmark_corpus.hpp"
#include <charconv>
#include <sstream>
#include <string_view>

namespace duomec::runtime {
std::vector<LargeAssemblyScenario> makeLargeAssemblyCorpus() {
  return {
    {"LA-01-10K", 5101001, 2, 10000, 2, 1, 2'000'000, 0, 0},
    {"LA-01-50K", 5101002, 2, 50000, 2, 1, 2'000'000, 0, 0},
    {"LA-02", 5102001, 5000, 5000, 2, 1, 500'000'000, 0, 0},
    {"LA-03", 5103001, 1500, 12000, 6, 1, 1'500'000'000, 0, 0},
    {"LA-04", 5104001, 50, 500, 3, 1, 8'000'000'000, 0, 0},
    {"LA-05", 5105001, 1000, 10000, 64, 1, 250'000'000, 0, 0},
    {"LA-06", 5106001, 1500, 12000, 6, 6, 3'000'000'000, 0, 0},
    {"LA-07", 5107001, 1500, 12000, 6, 1, 1'500'000'000, 50, 100}
  };
}
std::string encodeCorpus(const std::vector<LargeAssemblyScenario> &corpus) {
  std::ostringstream out; out << "DUOMEC_LA_CORPUS|1\n";
  for (const auto &s : corpus) out << s.id << '|' << s.seed << '|' << s.uniqueDefinitions << '|'
    << s.occurrences << '|' << s.maximumDepth << '|' << s.openTabs << '|'
    << s.estimatedGeometryBytes << '|' << s.networkLatencyMilliseconds << '|'
    << s.networkBandwidthMegabits << '\n';
  return out.str();
}
core::Result<std::vector<LargeAssemblyScenario>> decodeCorpus(const std::string &text) {
  std::istringstream input(text); std::string line;
  if (!std::getline(input, line) || line != "DUOMEC_LA_CORPUS|1")
    return core::Result<std::vector<LargeAssemblyScenario>>::failure({core::ErrorCode::invalid_argument, "invalid corpus header", "decodeCorpus"});
  std::vector<LargeAssemblyScenario> result;
  while (std::getline(input, line)) {
    if (line.empty()) continue;
    std::vector<std::string_view> fields; std::string_view view(line); std::size_t start = 0;
    while (true) { const auto pos = view.find('|', start); fields.push_back(view.substr(start, pos - start)); if (pos == std::string_view::npos) break; start = pos + 1; }
    if (fields.size() != 9 || fields[0].empty())
      return core::Result<std::vector<LargeAssemblyScenario>>::failure({core::ErrorCode::invalid_argument, "invalid corpus row", "decodeCorpus"});
    std::uint64_t values[8]{};
    for (std::size_t i = 1; i < fields.size(); ++i) {
      const auto [ptr, ec] = std::from_chars(fields[i].data(), fields[i].data() + fields[i].size(), values[i-1]);
      if (ec != std::errc{} || ptr != fields[i].data() + fields[i].size())
        return core::Result<std::vector<LargeAssemblyScenario>>::failure({core::ErrorCode::invalid_argument, "invalid corpus number", "decodeCorpus"});
    }
    result.push_back({std::string(fields[0]), values[0], values[1], values[2], static_cast<std::uint32_t>(values[3]), static_cast<std::uint32_t>(values[4]), values[5], static_cast<std::uint32_t>(values[6]), static_cast<std::uint32_t>(values[7])});
  }
  return core::Result<std::vector<LargeAssemblyScenario>>::success(std::move(result));
}
} // namespace duomec::runtime

#include "duomec/cad/features/feature_framework.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

namespace {
using namespace duomec::cad;
using namespace duomec::cad::features;
class Executor final : public IFeatureExecutor {
public:
  FeatureResult execute(const FeatureDefinition &,
                        const FeatureExecutionContext &context,
                        std::stop_token) override {
    checksum += static_cast<std::uint64_t>(context.parameters.size() +
                                           context.selections.size());
    FeatureResult result;
    result.status = FeatureStatus::success;
    return result;
  }
  std::uint64_t checksum{};
};
class Transaction final : public IFeatureTransaction {
public:
  duomec::core::Result<FeatureResult>
  commit(const FeatureDefinition &definition,
         const FeatureExecutionContext &context, IFeatureExecutor &executor,
         std::stop_token token) override {
    return duomec::core::Result<FeatureResult>::success(
        executor.execute(definition, context, token));
  }
};
FeatureDefinition definition() {
  FeatureDefinition result;
  result.type_key = "benchmark.feature";
  result.display_name = "Benchmark";
  result.parameter_schema.parameters = {
      {"length", "Length", ParameterType::scalar, ParameterDimension::length,
       0.01, 1.0e-9, 1.0, true, false}};
  result.collectors = {
      {"faces", "Faces", {{SelectionKind::face}, 1, 4, false}, true}};
  result.supported_operations = {BodyOperationMode::new_body};
  return result;
}
} // namespace

int main() {
  constexpr int iterations = 10000;
  Executor executor;
  Transaction transaction;
  const auto schema = definition();
  const auto started = std::chrono::steady_clock::now();
  for (int index = 0; index < iterations; ++index) {
    FeaturePreviewSession session(schema, executor, transaction);
    session.consume_preselection(
        {{"face:" + std::to_string(index), SelectionKind::face, "Face", true}});
    if (!session.preview())
      return 1;
  }
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - started);
  std::cout << "benchmark,iterations,total_ns,ns_per_iteration,checksum\n"
            << "feature_framework_preview," << iterations << ','
            << elapsed.count() << ',' << elapsed.count() / iterations << ','
            << executor.checksum << '\n';
  return executor.checksum == static_cast<std::uint64_t>(iterations * 2) ? 0
                                                                         : 2;
}

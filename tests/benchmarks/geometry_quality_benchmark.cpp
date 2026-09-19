#include "duomec/cad/geometry_quality/laws.hpp"
#include "duomec/cad/geometry_quality/quality.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

namespace {
using namespace duomec::cad::geometry_quality;
class Line final : public IBoundaryEvaluator {
public:
  explicit Line(double offset) : offset_(offset) {}
  duomec::core::Result<BoundarySample>
  evaluate(double parameter) const override {
    return duomec::core::Result<BoundarySample>::success(
        {parameter,
         {parameter, offset_, 0.0},
         Vector3d{1.0, 0.0, 0.0},
         Vector3d{0.0, 1.0, 0.0},
         0.0});
  }

private:
  double offset_{};
};
} // namespace

int main() {
  using namespace duomec::cad::geometry_quality;
  constexpr std::size_t law_iterations = 1'000'000;
  constexpr std::size_t continuity_iterations = 1'000;
  auto law = InterpolatedLaw::create(
      {{0.0, 0.01}, {0.2, 0.015}, {0.55, 0.008}, {0.8, 0.02}, {1.0, 0.012}});
  if (!law)
    return 1;
  volatile double checksum = 0.0;
  const auto law_start = std::chrono::steady_clock::now();
  for (std::size_t index = 0; index < law_iterations; ++index)
    checksum = checksum + law.value()->evaluate(
                              static_cast<double>(index % 1001) / 1000.0);
  const auto law_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - law_start);
  const Line first(0.0);
  const Line second(1.0e-7);
  const auto continuity_start = std::chrono::steady_clock::now();
  for (std::size_t index = 0; index < continuity_iterations; ++index) {
    auto result = ContinuityEvaluator::evaluate(
        first, second, ContinuityIntent::curvature, 65);
    if (!result || result.value().status != QualityStatus::pass)
      return 2;
    checksum = checksum + result.value().positional_gap_si.maximum;
  }
  const auto continuity_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now() - continuity_start);
  std::cout << "benchmark,iterations,total_ns,ns_per_iteration,checksum\n"
            << "interpolated_law," << law_iterations << ',' << law_time.count()
            << ',' << law_time.count() / law_iterations << ',' << checksum
            << '\n'
            << "continuity_65_samples," << continuity_iterations << ','
            << continuity_time.count() << ','
            << continuity_time.count() / continuity_iterations << ','
            << checksum << '\n';
  return checksum > 0.0 ? 0 : 3;
}

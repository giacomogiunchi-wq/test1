#include "duomec/cad/geometry_quality/laws.hpp"
#include "duomec/cad/geometry_quality/quality.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <numbers>

namespace {
using namespace duomec::cad::geometry_quality;

bool check(bool condition, const char *message) {
  if (!condition)
    std::cerr << "FAIL: " << message << '\n';
  return condition;
}
bool near(double first, double second, double tolerance = 1.0e-12) {
  return std::abs(first - second) <= tolerance;
}

class AnalyticBoundary final : public IBoundaryEvaluator {
public:
  using Function = std::function<BoundarySample(double)>;
  explicit AnalyticBoundary(Function function)
      : function_(std::move(function)) {}
  duomec::core::Result<BoundarySample>
  evaluate(double parameter) const override {
    return duomec::core::Result<BoundarySample>::success(function_(parameter));
  }

private:
  Function function_;
};
} // namespace

int main() {
  using namespace duomec::cad::geometry_quality;
  bool ok = true;

  auto linear =
      PiecewiseLinearLaw::create({{0.0, 0.01}, {0.25, 0.02}, {1.0, 0.05}});
  ok &= check(linear.has_value(), "create normalized piecewise law");
  ok &= check(near(linear.value()->evaluate(0.125), 0.015),
              "linear interpolation is deterministic");
  ok &= check(near(linear.value()->evaluate(-2.0), 0.01) &&
                  near(linear.value()->evaluate(2.0), 0.05),
              "law extrapolation clamps to endpoint stations");
  const std::string encoded = linear.value()->serialize();
  ok &= check(encoded == "duomec.scalar-law/1|linear|3|0,0.01|0.25,0.02|1,"
                         "0.050000000000000003",
              "canonical scalar law encoding is stable");
  auto decoded = deserialize_scalar_law(encoded);
  ok &= check(decoded && decoded.value()->serialize() == encoded,
              "scalar law serialization round trips byte-identically");
  ok &= check(!PiecewiseLinearLaw::create({{0.0, 1.0}, {0.0, 2.0}, {1.0, 3.0}}),
              "duplicate station parameters are rejected");
  ok &= check(!deserialize_scalar_law("duomec.scalar-law/9|linear|2|0,1|1,2"),
              "unknown law format versions are rejected");

  auto cubic =
      InterpolatedLaw::create({{0.0, 1.0}, {0.3, 2.0}, {0.7, 1.5}, {1.0, 3.0}});
  ok &= check(cubic && near(cubic.value()->evaluate(0.3), 2.0) &&
                  near(cubic.value()->evaluate(0.7), 1.5),
              "interpolated law passes exactly through stations");
  auto radius = RadiusLaw::create(cubic.value());
  ok &= check(radius && radius.value().serialize() ==
                            RadiusLaw::deserialize(radius.value().serialize())
                                .value()
                                .serialize(),
              "radius law preserves semantic serialization");
  auto nonpositive = PiecewiseLinearLaw::create({{0.0, 0.0}, {1.0, 1.0}});
  ok &= check(nonpositive && !RadiusLaw::create(nonpositive.value()),
              "radius law rejects non-positive SI values");
  auto scale = ScaleLaw::create(linear.value());
  auto twist = TwistLaw::create(linear.value());
  ok &= check(scale && twist &&
                  ScaleLaw::deserialize(scale.value().serialize()) &&
                  TwistLaw::deserialize(twist.value().serialize()),
              "scale and twist laws round trip");

  AnalyticBoundary line([](double parameter) {
    return BoundarySample{parameter,
                          {parameter, 0.0, 0.0},
                          Vector3d{1.0, 0.0, 0.0},
                          Vector3d{0.0, 1.0, 0.0},
                          0.0};
  });
  auto identical = ContinuityEvaluator::evaluate(
      line, line, ContinuityIntent::curvature, 33);
  ok &= check(
      identical && identical.value().status == QualityStatus::pass &&
          near(identical.value().positional_gap_si.maximum, 0.0) &&
          near(identical.value().angular_deviation_radians.maximum, 0.0) &&
          near(identical.value().curvature_deviation_inv_si.maximum, 0.0),
      "identical analytic boundaries measure G2 continuity");

  AnalyticBoundary offset([](double parameter) {
    return BoundarySample{parameter,
                          {parameter, 0.001, 0.0},
                          Vector3d{-1.0, 0.0, 0.0},
                          Vector3d{0.0, -1.0, 0.0},
                          0.0};
  });
  auto gap = ContinuityEvaluator::evaluate(line, offset,
                                           ContinuityIntent::position, 17,
                                           {.position_si = 0.0005,
                                            .angular_radians = 1.0e-3,
                                            .curvature_inv_si = 1.0e-2});
  ok &= check(gap && gap.value().status == QualityStatus::fail &&
                  near(gap.value().positional_gap_si.maximum, 0.001) &&
                  near(gap.value().positional_gap_si.mean, 0.001) &&
                  near(gap.value().positional_gap_si.rms, 0.001) &&
                  near(gap.value().angular_deviation_radians.maximum, 0.0),
              "constant gap fails position while anti-parallel normals are G1");

  const double angle = 0.02;
  AnalyticBoundary angled([angle](double parameter) {
    return BoundarySample{parameter,
                          {parameter, 0.0, 0.0},
                          Vector3d{std::cos(angle), std::sin(angle), 0.0},
                          std::nullopt,
                          std::nullopt};
  });
  AnalyticBoundary tangent_only([](double parameter) {
    return BoundarySample{parameter,
                          {parameter, 0.0, 0.0},
                          Vector3d{1.0, 0.0, 0.0},
                          std::nullopt,
                          std::nullopt};
  });
  auto angular = ContinuityEvaluator::evaluate(tangent_only, angled,
                                               ContinuityIntent::tangent, 9,
                                               {.position_si = 1.0e-9,
                                                .angular_radians = 0.01,
                                                .curvature_inv_si = 1.0e-2});
  ok &= check(angular && angular.value().status == QualityStatus::fail &&
                  near(angular.value().angular_deviation_radians.maximum, angle,
                       1.0e-12),
              "analytic tangent angle is measured in radians");
  auto unavailable = ContinuityEvaluator::evaluate(
      tangent_only, tangent_only, ContinuityIntent::curvature, 9);
  ok &= check(unavailable &&
                  unavailable.value().status == QualityStatus::unavailable,
              "missing curvature is reported rather than assumed G2");

  const BoundingBox3d box{{0.0, 0.0, 0.0}, {1.0, 2.0, 3.0}, true};
  auto curve = ComplexityReporter::curve(3, 4, 3, 2, false, false, box,
                                         {1.0e-6, 2.0e-6, 3.0e-6, false});
  ok &= check(curve && curve.value().approximate_payload_bytes == 132 &&
                  curve.value().tolerance.tolerance_grew,
              "curve complexity and tolerance growth are deterministic");
  auto surface =
      ComplexityReporter::surface(3, 2, 4, 5, 3, 4, 2, 3, true, false, false,
                                  box, {1.0e-6, 1.0e-6, 1.0e-6, true});
  ok &= check(surface && surface.value().approximate_payload_bytes == 724 &&
                  !surface.value().tolerance.tolerance_grew,
              "surface complexity estimate is deterministic");
  ok &= check(!ComplexityReporter::surface(5, 2, 4, 5, 3, 4, 2, 3, false, false,
                                           false, box, {}),
              "impossible degree/pole metadata is rejected");

  return ok ? 0 : 1;
}

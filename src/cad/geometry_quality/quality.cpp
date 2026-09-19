#include "duomec/cad/geometry_quality/quality.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace duomec::cad::geometry_quality {
namespace {
double squared_norm(const Vector3d &value) {
  return value.x * value.x + value.y * value.y + value.z * value.z;
}
double distance(const Point3d &first, const Point3d &second) {
  return std::sqrt(squared_norm(
      {first.x - second.x, first.y - second.y, first.z - second.z}));
}
std::optional<double> direction_angle(const Vector3d &first,
                                      const Vector3d &second) {
  const double first_norm = std::sqrt(squared_norm(first));
  const double second_norm = std::sqrt(squared_norm(second));
  if (!std::isfinite(first_norm) || !std::isfinite(second_norm) ||
      first_norm <= 1.0e-15 || second_norm <= 1.0e-15)
    return std::nullopt;
  const double dot =
      (first.x * second.x + first.y * second.y + first.z * second.z) /
      (first_norm * second_norm);
  // Tangent planes and unoriented curve tangents are geometrically continuous
  // for parallel or anti-parallel directions.
  return std::acos(std::clamp(std::abs(dot), 0.0, 1.0));
}

struct Accumulator {
  double maximum{-std::numeric_limits<double>::infinity()};
  double sum{};
  double squared_sum{};
  double parameter_at_maximum{};
  std::size_t count{};
  void add(double value, double parameter) {
    if (!std::isfinite(value))
      return;
    if (value > maximum) {
      maximum = value;
      parameter_at_maximum = parameter;
    }
    sum += value;
    squared_sum += value * value;
    ++count;
  }
  MetricMeasurement finish() const {
    if (count == 0)
      return {};
    return {true,
            maximum,
            sum / static_cast<double>(count),
            std::sqrt(squared_sum / static_cast<double>(count)),
            parameter_at_maximum,
            count};
  }
};

bool finite_nonnegative(double value) {
  return std::isfinite(value) && value >= 0.0;
}
bool valid_box(const BoundingBox3d &box) {
  if (!box.valid)
    return true;
  const double values[] = {box.minimum.x, box.minimum.y, box.minimum.z,
                           box.maximum.x, box.maximum.y, box.maximum.z};
  return std::ranges::all_of(
             values, [](double value) { return std::isfinite(value); }) &&
         box.minimum.x <= box.maximum.x && box.minimum.y <= box.maximum.y &&
         box.minimum.z <= box.maximum.z;
}
core::Result<bool> validate_tolerance(const ToleranceMetadata &tolerance) {
  if (!finite_nonnegative(tolerance.requested_si) ||
      !finite_nonnegative(tolerance.achieved_si) ||
      !finite_nonnegative(tolerance.maximum_entity_si))
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument,
         "complexity tolerance metadata must be finite and non-negative",
         {}});
  return core::Result<bool>::success(true);
}

std::size_t saturating_add(std::size_t first, std::size_t second) {
  if (second > std::numeric_limits<std::size_t>::max() - first)
    return std::numeric_limits<std::size_t>::max();
  return first + second;
}
std::size_t saturating_multiply(std::size_t first, std::size_t second) {
  if (first != 0 && second > std::numeric_limits<std::size_t>::max() / first)
    return std::numeric_limits<std::size_t>::max();
  return first * second;
}
} // namespace

core::Result<ContinuityMeasurement> ContinuityEvaluator::evaluate(
    const IBoundaryEvaluator &first, const IBoundaryEvaluator &second,
    ContinuityIntent requested, std::size_t sample_count,
    const ContinuityTolerances &tolerances) {
  if (sample_count < 2 || !finite_nonnegative(tolerances.position_si) ||
      !finite_nonnegative(tolerances.angular_radians) ||
      !finite_nonnegative(tolerances.curvature_inv_si))
    return core::Result<ContinuityMeasurement>::failure(
        {core::ErrorCode::invalid_argument,
         "continuity evaluation requires at least two samples and valid "
         "tolerances",
         {}});
  Accumulator position;
  Accumulator angle;
  Accumulator curvature;
  ContinuityMeasurement measurement;
  measurement.requested = requested;
  measurement.requested_samples = sample_count;
  for (std::size_t index = 0; index < sample_count; ++index) {
    const double parameter =
        static_cast<double>(index) / static_cast<double>(sample_count - 1);
    auto first_sample = first.evaluate(parameter);
    if (!first_sample)
      return core::Result<ContinuityMeasurement>::failure(first_sample.error());
    auto second_sample = second.evaluate(parameter);
    if (!second_sample)
      return core::Result<ContinuityMeasurement>::failure(
          second_sample.error());
    position.add(
        distance(first_sample.value().position, second_sample.value().position),
        parameter);
    std::optional<double> deviation;
    if (first_sample.value().normal && second_sample.value().normal)
      deviation = direction_angle(*first_sample.value().normal,
                                  *second_sample.value().normal);
    else if (first_sample.value().tangent && second_sample.value().tangent)
      deviation = direction_angle(*first_sample.value().tangent,
                                  *second_sample.value().tangent);
    if (deviation)
      angle.add(*deviation, parameter);
    if (first_sample.value().curvature_inv_si &&
        second_sample.value().curvature_inv_si)
      curvature.add(std::abs(*first_sample.value().curvature_inv_si -
                             *second_sample.value().curvature_inv_si),
                    parameter);
  }
  measurement.positional_gap_si = position.finish();
  measurement.angular_deviation_radians = angle.finish();
  measurement.curvature_deviation_inv_si = curvature.finish();
  bool sufficient = measurement.positional_gap_si.available;
  bool passed = sufficient &&
                measurement.positional_gap_si.maximum <= tolerances.position_si;
  if (requested >= ContinuityIntent::tangent) {
    sufficient = sufficient && measurement.angular_deviation_radians.available;
    passed = passed && measurement.angular_deviation_radians.available &&
             measurement.angular_deviation_radians.maximum <=
                 tolerances.angular_radians;
  }
  if (requested >= ContinuityIntent::curvature) {
    sufficient = sufficient && measurement.curvature_deviation_inv_si.available;
    passed = passed && measurement.curvature_deviation_inv_si.available &&
             measurement.curvature_deviation_inv_si.maximum <=
                 tolerances.curvature_inv_si;
  }
  if (!sufficient) {
    measurement.status = QualityStatus::unavailable;
    measurement.diagnostics.emplace_back(
        "requested continuity could not be measured from available samples");
  } else {
    measurement.status = passed ? QualityStatus::pass : QualityStatus::fail;
  }
  return core::Result<ContinuityMeasurement>::success(std::move(measurement));
}

core::Result<CurveComplexityMetrics> ComplexityReporter::curve(
    int degree, std::size_t pole_count, std::size_t knot_count,
    std::size_t span_count, bool rational, bool periodic,
    BoundingBox3d bounding_box, ToleranceMetadata tolerance) {
  auto valid_tolerance = validate_tolerance(tolerance);
  if (degree < 1 || pole_count < static_cast<std::size_t>(degree + 1) ||
      knot_count < 2 || span_count < 1 || !valid_box(bounding_box) ||
      !valid_tolerance)
    return core::Result<CurveComplexityMetrics>::failure(
        {core::ErrorCode::invalid_argument,
         "invalid curve complexity metadata",
         {}});
  tolerance.tolerance_grew =
      tolerance.achieved_si > tolerance.requested_si ||
      tolerance.maximum_entity_si > tolerance.requested_si;
  const std::size_t coordinates = rational ? 4U : 3U;
  std::size_t bytes =
      saturating_multiply(pole_count, coordinates * sizeof(double));
  bytes = saturating_add(
      bytes, saturating_multiply(knot_count, sizeof(double) + sizeof(int)));
  return core::Result<CurveComplexityMetrics>::success(
      {degree, pole_count, knot_count, span_count, rational, periodic, bytes,
       bounding_box, tolerance});
}

core::Result<SurfaceComplexityMetrics> ComplexityReporter::surface(
    int degree_u, int degree_v, std::size_t pole_count_u,
    std::size_t pole_count_v, std::size_t knot_count_u,
    std::size_t knot_count_v, std::size_t span_count_u,
    std::size_t span_count_v, bool rational, bool periodic_u, bool periodic_v,
    BoundingBox3d bounding_box, ToleranceMetadata tolerance) {
  auto valid_tolerance = validate_tolerance(tolerance);
  if (degree_u < 1 || degree_v < 1 ||
      pole_count_u < static_cast<std::size_t>(degree_u + 1) ||
      pole_count_v < static_cast<std::size_t>(degree_v + 1) ||
      knot_count_u < 2 || knot_count_v < 2 || span_count_u < 1 ||
      span_count_v < 1 || !valid_box(bounding_box) || !valid_tolerance)
    return core::Result<SurfaceComplexityMetrics>::failure(
        {core::ErrorCode::invalid_argument,
         "invalid surface complexity metadata",
         {}});
  tolerance.tolerance_grew =
      tolerance.achieved_si > tolerance.requested_si ||
      tolerance.maximum_entity_si > tolerance.requested_si;
  const std::size_t coordinates = rational ? 4U : 3U;
  const std::size_t poles = saturating_multiply(pole_count_u, pole_count_v);
  std::size_t bytes = saturating_multiply(
      poles, saturating_multiply(coordinates, sizeof(double)));
  bytes = saturating_add(
      bytes, saturating_multiply(saturating_add(knot_count_u, knot_count_v),
                                 sizeof(double) + sizeof(int)));
  return core::Result<SurfaceComplexityMetrics>::success(
      {degree_u, degree_v, pole_count_u, pole_count_v, knot_count_u,
       knot_count_v, span_count_u, span_count_v, rational, periodic_u,
       periodic_v, bytes, bounding_box, tolerance});
}

} // namespace duomec::cad::geometry_quality

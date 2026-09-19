#pragma once

#include "duomec/core/error.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace duomec::cad::geometry_quality {

struct Point3d {
  double x{};
  double y{};
  double z{};
  auto operator<=>(const Point3d &) const = default;
};

using Vector3d = Point3d;

enum class ContinuityIntent { position, tangent, curvature };
enum class QualityStatus { pass, warning, fail, unavailable };

struct BoundarySample {
  double parameter{};
  Point3d position;
  std::optional<Vector3d> tangent;
  std::optional<Vector3d> normal;
  std::optional<double> curvature_inv_si;
};

class IBoundaryEvaluator {
public:
  virtual ~IBoundaryEvaluator() = default;
  [[nodiscard]] virtual core::Result<BoundarySample>
  evaluate(double normalized_parameter) const = 0;
};

struct MetricMeasurement {
  bool available{};
  double maximum{};
  double mean{};
  double rms{};
  double parameter_at_maximum{};
  std::size_t valid_samples{};
  auto operator<=>(const MetricMeasurement &) const = default;
};

struct ContinuityTolerances {
  double position_si{1.0e-6};
  double angular_radians{1.0e-3};
  double curvature_inv_si{1.0e-2};
};

struct ContinuityMeasurement {
  ContinuityIntent requested{ContinuityIntent::position};
  QualityStatus status{QualityStatus::unavailable};
  std::size_t requested_samples{};
  MetricMeasurement positional_gap_si;
  MetricMeasurement angular_deviation_radians;
  MetricMeasurement curvature_deviation_inv_si;
  std::vector<std::string> diagnostics;
};

class ContinuityEvaluator {
public:
  [[nodiscard]] static core::Result<ContinuityMeasurement>
  evaluate(const IBoundaryEvaluator &first, const IBoundaryEvaluator &second,
           ContinuityIntent requested, std::size_t sample_count,
           const ContinuityTolerances &tolerances = {});
};

struct BoundingBox3d {
  Point3d minimum;
  Point3d maximum;
  bool valid{};
  auto operator<=>(const BoundingBox3d &) const = default;
};

struct ToleranceMetadata {
  double requested_si{};
  double achieved_si{};
  double maximum_entity_si{};
  bool tolerance_grew{};
  auto operator<=>(const ToleranceMetadata &) const = default;
};

struct CurveComplexityMetrics {
  int degree{};
  std::size_t pole_count{};
  std::size_t knot_count{};
  std::size_t span_count{};
  bool rational{};
  bool periodic{};
  std::size_t approximate_payload_bytes{};
  BoundingBox3d bounding_box;
  ToleranceMetadata tolerance;
  auto operator<=>(const CurveComplexityMetrics &) const = default;
};

struct SurfaceComplexityMetrics {
  int degree_u{};
  int degree_v{};
  std::size_t pole_count_u{};
  std::size_t pole_count_v{};
  std::size_t knot_count_u{};
  std::size_t knot_count_v{};
  std::size_t span_count_u{};
  std::size_t span_count_v{};
  bool rational{};
  bool periodic_u{};
  bool periodic_v{};
  std::size_t approximate_payload_bytes{};
  BoundingBox3d bounding_box;
  ToleranceMetadata tolerance;
  auto operator<=>(const SurfaceComplexityMetrics &) const = default;
};

class ComplexityReporter {
public:
  [[nodiscard]] static core::Result<CurveComplexityMetrics>
  curve(int degree, std::size_t pole_count, std::size_t knot_count,
        std::size_t span_count, bool rational, bool periodic,
        BoundingBox3d bounding_box, ToleranceMetadata tolerance);
  [[nodiscard]] static core::Result<SurfaceComplexityMetrics>
  surface(int degree_u, int degree_v, std::size_t pole_count_u,
          std::size_t pole_count_v, std::size_t knot_count_u,
          std::size_t knot_count_v, std::size_t span_count_u,
          std::size_t span_count_v, bool rational, bool periodic_u,
          bool periodic_v, BoundingBox3d bounding_box,
          ToleranceMetadata tolerance);
};

struct GeometryQualityReport {
  QualityStatus status{QualityStatus::unavailable};
  std::vector<ContinuityMeasurement> continuity;
  std::vector<CurveComplexityMetrics> curves;
  std::vector<SurfaceComplexityMetrics> surfaces;
  std::vector<std::string> diagnostics;
};

} // namespace duomec::cad::geometry_quality

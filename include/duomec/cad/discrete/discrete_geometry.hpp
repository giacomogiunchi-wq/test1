#pragma once

#include "duomec/cad/document/ids.hpp"
#include "duomec/cad/geometry_quality/quality.hpp"
#include "duomec/core/error.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace duomec::cad::discrete {

struct Transform3d {
  std::array<double, 16> matrix{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
  auto operator<=>(const Transform3d &) const = default;
};

struct ExternalAssetReference {
  std::string content_hash;
  std::filesystem::path relative_locator;
  std::string media_type;
  std::uintmax_t byte_size{};
  double source_length_unit_si{1.0};
  auto operator<=>(const ExternalAssetReference &) const = default;
};

struct DiscreteGeometryRevision {
  std::uint64_t sequence{};
  std::string content_hash;
  std::optional<std::string> parent_content_hash;
  auto operator<=>(const DiscreteGeometryRevision &) const = default;
};

enum class DiscreteDiagnosticCode {
  mesh_non_manifold,
  mesh_open_boundary,
  mesh_degenerate_triangles,
  mesh_repair_exceeded_tolerance,
  mesh_reduction_error_too_large,
  point_cloud_too_sparse,
  point_cloud_normals_unreliable,
  registration_low_overlap,
  registration_high_residual,
  segmentation_ambiguous,
  primitive_fit_low_confidence,
  primitive_fit_exceeds_tolerance,
  reconstruction_open_shell,
  reconstruction_invalid_brep,
  reconstruction_deviation_too_high,
  feature_recognition_ambiguous,
  feature_recognition_interaction,
  feature_reconstruction_mismatch
};

struct DiscreteDiagnostic {
  DiscreteDiagnosticCode code{};
  std::string message;
  std::vector<std::string> affected_references;
  std::string technical_details;
};

enum class ReverseEngineeringTolerancePreset {
  precision_machined_scan,
  general_mechanical_scan,
  coarse_scan,
  custom
};

struct ReverseEngineeringTolerancePolicy {
  double source_noise_si{};
  double merge_tolerance_si{};
  double primitive_fit_tolerance_si{};
  double curve_fit_tolerance_si{};
  double surface_fit_tolerance_si{};
  double sewing_tolerance_si{};
  double reconstruction_validation_tolerance_si{};

  [[nodiscard]] static ReverseEngineeringTolerancePolicy
  preset(ReverseEngineeringTolerancePreset preset);
  [[nodiscard]] core::Result<bool> validate() const;
  auto operator<=>(const ReverseEngineeringTolerancePolicy &) const = default;
};

struct Triangle {
  std::uint32_t first{};
  std::uint32_t second{};
  std::uint32_t third{};
  auto operator<=>(const Triangle &) const = default;
};

struct MeshBuffer {
  std::vector<geometry_quality::Point3d> vertices;
  std::vector<Triangle> triangles;
};

struct PointCloudBuffer {
  std::vector<geometry_quality::Point3d> points;
  std::vector<geometry_quality::Vector3d> normals;
  std::vector<std::array<std::uint8_t, 3>> colors;
};

struct MeshStatistics {
  std::size_t vertex_count{};
  std::size_t triangle_count{};
  std::size_t connected_component_count{};
  std::size_t boundary_edge_count{};
  std::size_t non_manifold_edge_count{};
  std::size_t degenerate_triangle_count{};
  std::size_t duplicated_vertex_estimate{};
  bool watertight{};
  bool oriented{};
  geometry_quality::BoundingBox3d bounding_box;
  std::optional<double> surface_area_si2;
  std::optional<double> enclosed_volume_si3;
  std::size_t memory_bytes{};
  auto operator<=>(const MeshStatistics &) const = default;
};

struct PointCloudStatistics {
  std::size_t point_count{};
  bool normals_present{};
  bool colors_present{};
  std::optional<double> estimated_spacing_si;
  geometry_quality::BoundingBox3d bounding_box;
  std::size_t outlier_count{};
  double outlier_ratio{};
  Transform3d source_transform;
  enum class CoordinatePrecision {
    float32,
    float64
  } coordinate_precision{CoordinatePrecision::float64};
  std::size_t memory_bytes{};
  auto operator<=>(const PointCloudStatistics &) const = default;
};

struct MeshSnapshot {
  DiscreteGeometryRevision revision;
  ExternalAssetReference source_asset;
  MeshStatistics statistics;
  std::shared_ptr<const MeshBuffer> data;
};

struct PointCloudSnapshot {
  DiscreteGeometryRevision revision;
  ExternalAssetReference source_asset;
  PointCloudStatistics statistics;
  std::shared_ptr<const PointCloudBuffer> data;
};

enum class ProcessingState { clean, dirty, failed, blocked, suppressed };
enum class MeshOperation { import, repair, reduce, smooth, segment, transform };
enum class PointCloudOperation {
  import,
  crop,
  remove_outliers,
  estimate_normals,
  register_cloud,
  segment,
  reconstruct_mesh,
  transform
};
using ProcessingParameter =
    std::variant<bool, std::int64_t, double, std::string>;

struct ProcessingFeatureRecord {
  DiscreteFeatureId id{DiscreteFeatureId::generate()};
  std::string name;
  std::map<std::string, ProcessingParameter, std::less<>> parameters;
  bool enabled{true};
  bool cacheable{true};
  ProcessingState state{ProcessingState::dirty};
  std::optional<DiscreteGeometryRevision> input_revision;
  std::optional<DiscreteGeometryRevision> output_revision;
  std::string cache_key;
  std::vector<DiscreteDiagnostic> diagnostics;
};

class MeshFeature {
public:
  MeshFeature(MeshOperation operation, ProcessingFeatureRecord record);
  [[nodiscard]] MeshOperation operation() const noexcept;
  [[nodiscard]] const ProcessingFeatureRecord &record() const noexcept;

private:
  friend class MeshProcessingPipeline;
  MeshOperation operation_;
  ProcessingFeatureRecord record_;
};

class PointCloudFeature {
public:
  PointCloudFeature(PointCloudOperation operation,
                    ProcessingFeatureRecord record);
  [[nodiscard]] PointCloudOperation operation() const noexcept;
  [[nodiscard]] const ProcessingFeatureRecord &record() const noexcept;

private:
  friend class PointCloudProcessingPipeline;
  PointCloudOperation operation_;
  ProcessingFeatureRecord record_;
};

class MeshProcessingPipeline {
public:
  MeshProcessingPipeline() = default;
  explicit MeshProcessingPipeline(std::vector<MeshFeature> features);
  core::Result<DiscreteFeatureId> append(
      MeshOperation operation, std::string name,
      std::map<std::string, ProcessingParameter, std::less<>> parameters = {});
  core::Result<bool> set_parameter(DiscreteFeatureId feature, std::string key,
                                   ProcessingParameter value);
  core::Result<bool> set_suppressed(DiscreteFeatureId feature, bool suppressed);
  core::Result<bool> accept_result(DiscreteFeatureId feature,
                                   DiscreteGeometryRevision output,
                                   std::string cache_key);
  [[nodiscard]] const std::vector<MeshFeature> &features() const noexcept;

private:
  void invalidate_from(std::size_t index);
  std::vector<MeshFeature> features_;
};

class PointCloudProcessingPipeline {
public:
  PointCloudProcessingPipeline() = default;
  explicit PointCloudProcessingPipeline(
      std::vector<PointCloudFeature> features);
  core::Result<DiscreteFeatureId> append(
      PointCloudOperation operation, std::string name,
      std::map<std::string, ProcessingParameter, std::less<>> parameters = {});
  core::Result<bool> set_parameter(DiscreteFeatureId feature, std::string key,
                                   ProcessingParameter value);
  core::Result<bool> set_suppressed(DiscreteFeatureId feature, bool suppressed);
  core::Result<bool> accept_result(DiscreteFeatureId feature,
                                   DiscreteGeometryRevision output,
                                   std::string cache_key);
  [[nodiscard]] const std::vector<PointCloudFeature> &features() const noexcept;

private:
  void invalidate_from(std::size_t index);
  std::vector<PointCloudFeature> features_;
};

class MeshBody {
public:
  MeshBody(MeshBodyId id, std::string name, MeshSnapshot source,
           MeshProcessingPipeline history = {});
  [[nodiscard]] MeshBodyId id() const;
  [[nodiscard]] const std::string &name() const noexcept;
  [[nodiscard]] const MeshSnapshot &source() const noexcept;
  [[nodiscard]] MeshProcessingPipeline &history() noexcept;
  [[nodiscard]] const MeshProcessingPipeline &history() const noexcept;

private:
  MeshBodyId id_;
  std::string name_;
  MeshSnapshot source_;
  MeshProcessingPipeline history_;
};

class PointCloudBody {
public:
  PointCloudBody(PointCloudBodyId id, std::string name,
                 PointCloudSnapshot source,
                 PointCloudProcessingPipeline history = {});
  [[nodiscard]] PointCloudBodyId id() const;
  [[nodiscard]] const std::string &name() const noexcept;
  [[nodiscard]] const PointCloudSnapshot &source() const noexcept;
  [[nodiscard]] PointCloudProcessingPipeline &history() noexcept;
  [[nodiscard]] const PointCloudProcessingPipeline &history() const noexcept;

private:
  PointCloudBodyId id_;
  std::string name_;
  PointCloudSnapshot source_;
  PointCloudProcessingPipeline history_;
};

using ProgressCallback =
    std::function<void(double fraction, std::string_view stage)>;

class IMeshKernel {
public:
  virtual ~IMeshKernel() = default;
  virtual core::Result<MeshSnapshot>
  import_file(const std::filesystem::path &path, double source_length_unit_si,
              std::stop_token cancellation,
              const ProgressCallback &progress = {}) = 0;
};

class IPointCloudKernel {
public:
  virtual ~IPointCloudKernel() = default;
  virtual core::Result<PointCloudSnapshot>
  import_file(const std::filesystem::path &path, double source_length_unit_si,
              std::stop_token cancellation,
              const ProgressCallback &progress = {}) = 0;
};

struct DiscreteDocumentState {
  std::vector<MeshBody> mesh_bodies;
  std::vector<PointCloudBody> point_cloud_bodies;
};

class DiscreteHistoryCodec {
public:
  [[nodiscard]] static core::Result<std::string>
  encode(const DiscreteDocumentState &state);
  [[nodiscard]] static core::Result<DiscreteDocumentState>
  decode(std::string_view encoded);
};

} // namespace duomec::cad::discrete

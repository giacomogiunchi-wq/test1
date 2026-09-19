#include "duomec/cad/discrete/discrete_geometry.hpp"

#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace duomec::cad::discrete {
namespace {
constexpr std::size_t kMaximumMetadataRecords = 1'000'000;
core::Error codec_error(std::string message) {
  return {core::ErrorCode::io_failure, std::move(message), "DuomecDiscrete/1"};
}
void write_revision(std::ostream &output,
                    const DiscreteGeometryRevision &revision) {
  output << revision.sequence << ' ' << std::quoted(revision.content_hash)
         << ' ' << revision.parent_content_hash.has_value();
  if (revision.parent_content_hash)
    output << ' ' << std::quoted(*revision.parent_content_hash);
  output << '\n';
}
bool read_revision(std::istream &input, DiscreteGeometryRevision &revision) {
  bool has_parent{};
  if (!(input >> revision.sequence >> std::quoted(revision.content_hash) >>
        has_parent))
    return false;
  if (has_parent) {
    std::string parent;
    if (!(input >> std::quoted(parent)))
      return false;
    revision.parent_content_hash = std::move(parent);
  } else {
    revision.parent_content_hash.reset();
  }
  return true;
}
void write_asset(std::ostream &output, const ExternalAssetReference &asset) {
  output << std::quoted(asset.content_hash) << ' '
         << std::quoted(asset.relative_locator.generic_string()) << ' '
         << std::quoted(asset.media_type) << ' ' << asset.byte_size << ' '
         << asset.source_length_unit_si << '\n';
}
bool read_asset(std::istream &input, ExternalAssetReference &asset) {
  std::string locator;
  if (!(input >> std::quoted(asset.content_hash) >> std::quoted(locator) >>
        std::quoted(asset.media_type) >> asset.byte_size >>
        asset.source_length_unit_si))
    return false;
  asset.relative_locator = std::filesystem::path(locator);
  return true;
}
void write_box(std::ostream &output,
               const geometry_quality::BoundingBox3d &box) {
  output << box.valid << ' ' << box.minimum.x << ' ' << box.minimum.y << ' '
         << box.minimum.z << ' ' << box.maximum.x << ' ' << box.maximum.y << ' '
         << box.maximum.z << '\n';
}
bool read_box(std::istream &input, geometry_quality::BoundingBox3d &box) {
  return static_cast<bool>(input >> box.valid >> box.minimum.x >>
                           box.minimum.y >> box.minimum.z >> box.maximum.x >>
                           box.maximum.y >> box.maximum.z);
}
void write_optional(std::ostream &output, const std::optional<double> &value) {
  output << value.has_value();
  if (value)
    output << ' ' << *value;
  output << '\n';
}
bool read_optional(std::istream &input, std::optional<double> &value) {
  bool present{};
  if (!(input >> present))
    return false;
  if (present) {
    double item{};
    if (!(input >> item))
      return false;
    value = item;
  } else {
    value.reset();
  }
  return true;
}
void write_parameter(std::ostream &output, std::string_view key,
                     const ProcessingParameter &value) {
  output << std::quoted(std::string(key)) << ' ' << value.index() << ' ';
  std::visit(
      [&output](const auto &item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::string>)
          output << std::quoted(item);
        else
          output << item;
      },
      value);
  output << '\n';
}
bool read_parameter(std::istream &input, std::string &key,
                    ProcessingParameter &value) {
  std::size_t type{};
  if (!(input >> std::quoted(key) >> type))
    return false;
  switch (type) {
  case 0: {
    bool item{};
    if (!(input >> item))
      return false;
    value = item;
    return true;
  }
  case 1: {
    std::int64_t item{};
    if (!(input >> item))
      return false;
    value = item;
    return true;
  }
  case 2: {
    double item{};
    if (!(input >> item))
      return false;
    value = item;
    return true;
  }
  case 3: {
    std::string item;
    if (!(input >> std::quoted(item)))
      return false;
    value = std::move(item);
    return true;
  }
  default:
    return false;
  }
}
void write_feature_record(std::ostream &output,
                          const ProcessingFeatureRecord &record) {
  output << std::quoted(record.id.value()) << ' ' << std::quoted(record.name)
         << ' ' << record.enabled << ' ' << record.cacheable << ' '
         << static_cast<int>(record.state) << ' '
         << std::quoted(record.cache_key) << ' ' << record.parameters.size()
         << ' ' << record.diagnostics.size() << '\n';
  output << record.input_revision.has_value() << '\n';
  if (record.input_revision)
    write_revision(output, *record.input_revision);
  output << record.output_revision.has_value() << '\n';
  if (record.output_revision)
    write_revision(output, *record.output_revision);
  for (const auto &[key, value] : record.parameters)
    write_parameter(output, key, value);
  for (const auto &diagnostic : record.diagnostics) {
    output << static_cast<int>(diagnostic.code) << ' '
           << std::quoted(diagnostic.message) << ' '
           << std::quoted(diagnostic.technical_details) << ' '
           << diagnostic.affected_references.size() << '\n';
    for (const auto &reference : diagnostic.affected_references)
      output << std::quoted(reference) << '\n';
  }
}
core::Result<ProcessingFeatureRecord> read_feature_record(std::istream &input) {
  std::string id_text;
  ProcessingFeatureRecord record;
  int state{};
  std::size_t parameter_count{};
  std::size_t diagnostic_count{};
  if (!(input >> std::quoted(id_text) >> std::quoted(record.name) >>
        record.enabled >> record.cacheable >> state >>
        std::quoted(record.cache_key) >> parameter_count >> diagnostic_count) ||
      parameter_count > kMaximumMetadataRecords ||
      diagnostic_count > kMaximumMetadataRecords)
    return core::Result<ProcessingFeatureRecord>::failure(
        codec_error("invalid processing feature header"));
  auto id = DiscreteFeatureId::parse(id_text);
  if (!id || state < static_cast<int>(ProcessingState::clean) ||
      state > static_cast<int>(ProcessingState::suppressed))
    return core::Result<ProcessingFeatureRecord>::failure(
        codec_error("invalid processing feature identity or state"));
  record.id = id.value();
  record.state = static_cast<ProcessingState>(state);
  bool has_input{};
  if (!(input >> has_input))
    return core::Result<ProcessingFeatureRecord>::failure(
        codec_error("missing input revision flag"));
  if (has_input) {
    DiscreteGeometryRevision revision;
    if (!read_revision(input, revision))
      return core::Result<ProcessingFeatureRecord>::failure(
          codec_error("invalid input revision"));
    record.input_revision = std::move(revision);
  }
  bool has_output{};
  if (!(input >> has_output))
    return core::Result<ProcessingFeatureRecord>::failure(
        codec_error("missing output revision flag"));
  if (has_output) {
    DiscreteGeometryRevision revision;
    if (!read_revision(input, revision))
      return core::Result<ProcessingFeatureRecord>::failure(
          codec_error("invalid output revision"));
    record.output_revision = std::move(revision);
  }
  if ((record.enabled && record.state == ProcessingState::suppressed) ||
      (!record.enabled && record.state != ProcessingState::suppressed) ||
      (record.state == ProcessingState::clean && !record.output_revision))
    return core::Result<ProcessingFeatureRecord>::failure(
        codec_error("inconsistent processing feature state"));
  for (std::size_t index = 0; index < parameter_count; ++index) {
    std::string key;
    ProcessingParameter value;
    if (!read_parameter(input, key, value) || key.empty() ||
        record.parameters.contains(key))
      return core::Result<ProcessingFeatureRecord>::failure(
          codec_error("invalid or duplicate processing parameter"));
    record.parameters.emplace(std::move(key), std::move(value));
  }
  for (std::size_t index = 0; index < diagnostic_count; ++index) {
    int code{};
    DiscreteDiagnostic diagnostic;
    std::size_t reference_count{};
    if (!(input >> code >> std::quoted(diagnostic.message) >>
          std::quoted(diagnostic.technical_details) >> reference_count) ||
        code < static_cast<int>(DiscreteDiagnosticCode::mesh_non_manifold) ||
        code > static_cast<int>(
                   DiscreteDiagnosticCode::feature_reconstruction_mismatch) ||
        reference_count > kMaximumMetadataRecords)
      return core::Result<ProcessingFeatureRecord>::failure(
          codec_error("invalid feature diagnostic"));
    diagnostic.code = static_cast<DiscreteDiagnosticCode>(code);
    diagnostic.affected_references.reserve(reference_count);
    for (std::size_t reference = 0; reference < reference_count; ++reference) {
      std::string value;
      if (!(input >> std::quoted(value)))
        return core::Result<ProcessingFeatureRecord>::failure(
            codec_error("invalid diagnostic reference"));
      diagnostic.affected_references.push_back(std::move(value));
    }
    record.diagnostics.push_back(std::move(diagnostic));
  }
  return core::Result<ProcessingFeatureRecord>::success(std::move(record));
}
void write_mesh_statistics(std::ostream &output,
                           const MeshStatistics &statistics) {
  output << statistics.vertex_count << ' ' << statistics.triangle_count << ' '
         << statistics.connected_component_count << ' '
         << statistics.boundary_edge_count << ' '
         << statistics.non_manifold_edge_count << ' '
         << statistics.degenerate_triangle_count << ' '
         << statistics.duplicated_vertex_estimate << ' '
         << statistics.watertight << ' ' << statistics.oriented << ' '
         << statistics.memory_bytes << '\n';
  write_box(output, statistics.bounding_box);
  write_optional(output, statistics.surface_area_si2);
  write_optional(output, statistics.enclosed_volume_si3);
}
bool read_mesh_statistics(std::istream &input, MeshStatistics &statistics) {
  return static_cast<bool>(
             input >> statistics.vertex_count >> statistics.triangle_count >>
             statistics.connected_component_count >>
             statistics.boundary_edge_count >>
             statistics.non_manifold_edge_count >>
             statistics.degenerate_triangle_count >>
             statistics.duplicated_vertex_estimate >> statistics.watertight >>
             statistics.oriented >> statistics.memory_bytes) &&
         read_box(input, statistics.bounding_box) &&
         read_optional(input, statistics.surface_area_si2) &&
         read_optional(input, statistics.enclosed_volume_si3);
}
void write_point_statistics(std::ostream &output,
                            const PointCloudStatistics &statistics) {
  output << statistics.point_count << ' ' << statistics.normals_present << ' '
         << statistics.colors_present << ' ' << statistics.outlier_count << ' '
         << statistics.outlier_ratio << ' '
         << static_cast<int>(statistics.coordinate_precision) << ' '
         << statistics.memory_bytes << '\n';
  write_optional(output, statistics.estimated_spacing_si);
  write_box(output, statistics.bounding_box);
  for (const double value : statistics.source_transform.matrix)
    output << value << ' ';
  output << '\n';
}
bool read_point_statistics(std::istream &input,
                           PointCloudStatistics &statistics) {
  int precision{};
  if (!(input >> statistics.point_count >> statistics.normals_present >>
        statistics.colors_present >> statistics.outlier_count >>
        statistics.outlier_ratio >> precision >> statistics.memory_bytes) ||
      precision < 0 || precision > 1 ||
      !read_optional(input, statistics.estimated_spacing_si) ||
      !read_box(input, statistics.bounding_box))
    return false;
  statistics.coordinate_precision =
      static_cast<PointCloudStatistics::CoordinatePrecision>(precision);
  for (double &value : statistics.source_transform.matrix)
    if (!(input >> value))
      return false;
  return true;
}
} // namespace

core::Result<std::string>
DiscreteHistoryCodec::encode(const DiscreteDocumentState &state) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(std::numeric_limits<double>::max_digits10);
  output << "DUOMEC_DISCRETE 1\n"
         << state.mesh_bodies.size() << ' ' << state.point_cloud_bodies.size()
         << '\n';
  for (const auto &body : state.mesh_bodies) {
    output << "MESH " << std::quoted(body.id().value()) << ' '
           << std::quoted(body.name()) << '\n';
    write_revision(output, body.source().revision);
    write_asset(output, body.source().source_asset);
    write_mesh_statistics(output, body.source().statistics);
    output << body.history().features().size() << '\n';
    for (const auto &feature : body.history().features()) {
      output << static_cast<int>(feature.operation()) << '\n';
      write_feature_record(output, feature.record());
    }
  }
  for (const auto &body : state.point_cloud_bodies) {
    output << "POINT " << std::quoted(body.id().value()) << ' '
           << std::quoted(body.name()) << '\n';
    write_revision(output, body.source().revision);
    write_asset(output, body.source().source_asset);
    write_point_statistics(output, body.source().statistics);
    output << body.history().features().size() << '\n';
    for (const auto &feature : body.history().features()) {
      output << static_cast<int>(feature.operation()) << '\n';
      write_feature_record(output, feature.record());
    }
  }
  output << "END\n";
  return core::Result<std::string>::success(output.str());
}

core::Result<DiscreteDocumentState>
DiscreteHistoryCodec::decode(std::string_view encoded) {
  std::istringstream input{std::string(encoded)};
  input.imbue(std::locale::classic());
  std::string magic;
  int version{};
  std::size_t mesh_count{};
  std::size_t point_count{};
  if (!(input >> magic >> version >> mesh_count >> point_count) ||
      magic != "DUOMEC_DISCRETE" || version != 1 ||
      mesh_count > kMaximumMetadataRecords ||
      point_count > kMaximumMetadataRecords)
    return core::Result<DiscreteDocumentState>::failure(
        codec_error("invalid discrete history header"));
  DiscreteDocumentState state;
  state.mesh_bodies.reserve(mesh_count);
  state.point_cloud_bodies.reserve(point_count);
  for (std::size_t body_index = 0; body_index < mesh_count; ++body_index) {
    std::string marker;
    std::string id_text;
    std::string name;
    if (!(input >> marker >> std::quoted(id_text) >> std::quoted(name)) ||
        marker != "MESH")
      return core::Result<DiscreteDocumentState>::failure(
          codec_error("invalid mesh body header"));
    auto id = MeshBodyId::parse(id_text);
    MeshSnapshot snapshot;
    if (!id || !read_revision(input, snapshot.revision) ||
        !read_asset(input, snapshot.source_asset) ||
        !read_mesh_statistics(input, snapshot.statistics))
      return core::Result<DiscreteDocumentState>::failure(
          codec_error("invalid mesh body metadata"));
    std::size_t feature_count{};
    if (!(input >> feature_count) || feature_count > kMaximumMetadataRecords)
      return core::Result<DiscreteDocumentState>::failure(
          codec_error("missing mesh feature count"));
    std::vector<MeshFeature> features;
    features.reserve(feature_count);
    for (std::size_t index = 0; index < feature_count; ++index) {
      int operation{};
      if (!(input >> operation) || operation < 0 ||
          operation > static_cast<int>(MeshOperation::transform))
        return core::Result<DiscreteDocumentState>::failure(
            codec_error("invalid mesh operation"));
      auto record = read_feature_record(input);
      if (!record)
        return core::Result<DiscreteDocumentState>::failure(record.error());
      features.emplace_back(static_cast<MeshOperation>(operation),
                            std::move(record).value());
    }
    state.mesh_bodies.emplace_back(id.value(), std::move(name),
                                   std::move(snapshot),
                                   MeshProcessingPipeline(std::move(features)));
  }
  for (std::size_t body_index = 0; body_index < point_count; ++body_index) {
    std::string marker;
    std::string id_text;
    std::string name;
    if (!(input >> marker >> std::quoted(id_text) >> std::quoted(name)) ||
        marker != "POINT")
      return core::Result<DiscreteDocumentState>::failure(
          codec_error("invalid point-cloud body header"));
    auto id = PointCloudBodyId::parse(id_text);
    PointCloudSnapshot snapshot;
    if (!id || !read_revision(input, snapshot.revision) ||
        !read_asset(input, snapshot.source_asset) ||
        !read_point_statistics(input, snapshot.statistics))
      return core::Result<DiscreteDocumentState>::failure(
          codec_error("invalid point-cloud body metadata"));
    std::size_t feature_count{};
    if (!(input >> feature_count) || feature_count > kMaximumMetadataRecords)
      return core::Result<DiscreteDocumentState>::failure(
          codec_error("missing point-cloud feature count"));
    std::vector<PointCloudFeature> features;
    features.reserve(feature_count);
    for (std::size_t index = 0; index < feature_count; ++index) {
      int operation{};
      if (!(input >> operation) || operation < 0 ||
          operation > static_cast<int>(PointCloudOperation::transform))
        return core::Result<DiscreteDocumentState>::failure(
            codec_error("invalid point-cloud operation"));
      auto record = read_feature_record(input);
      if (!record)
        return core::Result<DiscreteDocumentState>::failure(record.error());
      features.emplace_back(static_cast<PointCloudOperation>(operation),
                            std::move(record).value());
    }
    state.point_cloud_bodies.emplace_back(
        id.value(), std::move(name), std::move(snapshot),
        PointCloudProcessingPipeline(std::move(features)));
  }
  std::string end;
  if (!(input >> end) || end != "END")
    return core::Result<DiscreteDocumentState>::failure(
        codec_error("missing discrete history terminator"));
  input >> std::ws;
  if (!input.eof())
    return core::Result<DiscreteDocumentState>::failure(
        codec_error("trailing discrete history data"));
  return core::Result<DiscreteDocumentState>::success(std::move(state));
}

} // namespace duomec::cad::discrete

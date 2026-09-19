#include "duomec/cad/discrete/discrete_geometry.hpp"
#include "native_discrete_kernel.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stop_token>

namespace {
using namespace duomec::cad;
using namespace duomec::cad::discrete;
using namespace duomec::adapters::discrete;
bool check(bool condition, const char *message) {
  if (!condition)
    std::cerr << "FAIL: " << message << '\n';
  return condition;
}
bool near(double first, double second, double tolerance = 1.0e-12) {
  return std::abs(first - second) <= tolerance;
}
} // namespace

int main() {
  bool ok = true;
  const auto source = std::filesystem::path(DUOMEC_TEST_SOURCE_DIR);
  NativeObjMeshKernel mesh_kernel;
  double mesh_progress = 0.0;
  auto mesh = mesh_kernel.import_file(
      source / "tests/golden_models/m4a_cube.obj", 1.0, {},
      [&](double progress, std::string_view) { mesh_progress = progress; });
  ok &= check(mesh.has_value(), "import OBJ mesh");
  if (!mesh)
    return 1;
  ok &= check(mesh.value().statistics.vertex_count == 8 &&
                  mesh.value().statistics.triangle_count == 12 &&
                  mesh.value().statistics.connected_component_count == 1,
              "mesh counts and connectivity");
  ok &= check(mesh.value().statistics.watertight &&
                  mesh.value().statistics.oriented &&
                  mesh.value().statistics.boundary_edge_count == 0 &&
                  mesh.value().statistics.non_manifold_edge_count == 0 &&
                  mesh.value().statistics.degenerate_triangle_count == 0,
              "closed cube topology statistics");
  ok &= check(mesh.value().statistics.surface_area_si2 &&
                  near(*mesh.value().statistics.surface_area_si2, 6.0) &&
                  mesh.value().statistics.enclosed_volume_si3 &&
                  near(*mesh.value().statistics.enclosed_volume_si3, 1.0),
              "mesh area and enclosed volume use SI");
  ok &= check(mesh_progress == 1.0 && mesh.value().data.use_count() == 1,
              "import progress and immutable snapshot ownership");

  NativeXyzPointCloudKernel point_kernel;
  auto points = point_kernel.import_file(
      source / "tests/golden_models/m4a_points.xyz", 0.001, {}, {});
  ok &= check(points && points.value().statistics.point_count == 4,
              "import XYZ point cloud");
  if (!points)
    return 2;
  ok &=
      check(near(points.value().statistics.bounding_box.maximum.x, 0.01) &&
                near(points.value().statistics.bounding_box.maximum.y, 0.02) &&
                near(points.value().statistics.bounding_box.maximum.z, 0.03),
            "point-cloud source units convert explicitly to SI");
  ok &= check(!points.value().statistics.normals_present &&
                  !points.value().statistics.colors_present &&
                  points.value().statistics.memory_bytes ==
                      4 * sizeof(duomec::cad::geometry_quality::Point3d),
              "point metadata and memory estimate");

  MeshProcessingPipeline mesh_history;
  auto imported = mesh_history.append(MeshOperation::import, "Imported mesh");
  auto reduced =
      mesh_history.append(MeshOperation::reduce, "Reduce", {{"ratio", 0.5}});
  auto smooth = mesh_history.append(MeshOperation::smooth, "Smooth",
                                    {{"iterations", std::int64_t{3}}});
  ok &= check(imported && reduced && smooth, "create mesh processing history");
  ok &= check(
      mesh_history.accept_result(
          imported.value(), {1, "rev-import", std::nullopt}, "cache-import") &&
          mesh_history.accept_result(reduced.value(),
                                     {2, "rev-reduce", "rev-import"},
                                     "cache-reduce") &&
          mesh_history.accept_result(
              smooth.value(), {3, "rev-smooth", "rev-reduce"}, "cache-smooth"),
      "accept sequential cached revisions");
  ok &= check(
      mesh_history.set_parameter(reduced.value(), "ratio", 0.4) &&
          mesh_history.features()[0].record().state == ProcessingState::clean &&
          mesh_history.features()[1].record().state == ProcessingState::dirty &&
          mesh_history.features()[2].record().state == ProcessingState::dirty,
      "editing invalidates only the feature and downstream history");
  ok &= check(mesh_history.set_suppressed(reduced.value(), true) &&
                  mesh_history.features()[1].record().state ==
                      ProcessingState::suppressed,
              "processing features are suppressible");
  ok &= check(mesh_history.set_suppressed(reduced.value(), false) &&
                  mesh_history.features()[1].record().state ==
                      ProcessingState::dirty,
              "unsuppressed features return to dirty state");

  const auto tolerances = ReverseEngineeringTolerancePolicy::preset(
      ReverseEngineeringTolerancePreset::general_mechanical_scan);
  ok &= check(tolerances.validate() &&
                  near(tolerances.primitive_fit_tolerance_si, 1.0e-4),
              "named tolerance preset exposes explicit SI values");
  auto invalid_tolerances = tolerances;
  invalid_tolerances.sewing_tolerance_si = 0.0;
  ok &= check(!invalid_tolerances.validate(),
              "invalid tolerance policy is rejected explicitly");

  PointCloudProcessingPipeline point_history;
  auto point_import =
      point_history.append(PointCloudOperation::import, "Imported scan");
  auto normals =
      point_history.append(PointCloudOperation::estimate_normals,
                           "Estimate normals", {{"radius_si", 0.002}});
  ok &= check(point_import && normals, "create point-cloud processing history");

  DiscreteDocumentState state;
  state.mesh_bodies.emplace_back(MeshBodyId::generate(), "Cube", mesh.value(),
                                 std::move(mesh_history));
  state.point_cloud_bodies.emplace_back(PointCloudBodyId::generate(), "Scan",
                                        points.value(),
                                        std::move(point_history));
  auto encoded = DiscreteHistoryCodec::encode(state);
  auto decoded = encoded ? DiscreteHistoryCodec::decode(encoded.value())
                         : duomec::core::Result<DiscreteDocumentState>::failure(
                               encoded.error());
  ok &= check(decoded && decoded.value().mesh_bodies.size() == 1 &&
                  decoded.value().point_cloud_bodies.size() == 1,
              "metadata and feature histories reload");
  if (decoded) {
    ok &= check(
        decoded.value().mesh_bodies[0].id() == state.mesh_bodies[0].id() &&
            decoded.value().mesh_bodies[0].source().statistics ==
                state.mesh_bodies[0].source().statistics &&
            decoded.value().mesh_bodies[0].history().features().size() == 3 &&
            !decoded.value().mesh_bodies[0].source().data,
        "codec preserves identity/statistics/history but not raw asset");
    auto reencoded = DiscreteHistoryCodec::encode(decoded.value());
    ok &= check(reencoded && reencoded.value() == encoded.value(),
                "discrete metadata serialization is canonical");
  }
  ok &= check(!DiscreteHistoryCodec::decode("DUOMEC_DISCRETE 99\n"),
              "unknown discrete schema is rejected");

  std::stop_source cancelled;
  cancelled.request_stop();
  ok &= check(
      !point_kernel.import_file(source / "tests/golden_models/m4a_points.xyz",
                                1.0, cancelled.get_token(), {}),
      "import cancellation returns a structured error");
  return ok ? 0 : 1;
}

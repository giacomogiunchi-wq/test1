#include "duomec/cad/discrete/discrete_geometry.hpp"
#include "native_discrete_kernel.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stop_token>
#ifdef __linux__
#include <sys/resource.h>
#endif

namespace {
std::uint64_t peak_rss_bytes() {
#ifdef __linux__
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) == 0)
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
  return 0;
}
} // namespace

int main() {
  using namespace duomec::adapters::discrete;
  using namespace duomec::cad;
  using namespace duomec::cad::discrete;
  constexpr std::size_t grid = 708;
  constexpr std::size_t point_count = 1'000'000;
  const auto directory = std::filesystem::temp_directory_path();
  const auto mesh_path = directory / "duomec_m4a_1m_triangles.obj";
  const auto point_path = directory / "duomec_m4a_1m_points.xyz";
  {
    std::ofstream mesh(mesh_path);
    if (!mesh)
      return 1;
    for (std::size_t y = 0; y < grid; ++y)
      for (std::size_t x = 0; x < grid; ++x)
        mesh << "v " << x << ' ' << y << " 0\n";
    for (std::size_t y = 0; y + 1 < grid; ++y) {
      for (std::size_t x = 0; x + 1 < grid; ++x) {
        const std::size_t first = y * grid + x + 1;
        const std::size_t next = first + grid;
        mesh << "f " << first << ' ' << first + 1 << ' ' << next + 1 << '\n'
             << "f " << first << ' ' << next + 1 << ' ' << next << '\n';
      }
    }
  }
  {
    std::ofstream points(point_path);
    if (!points)
      return 2;
    for (std::size_t index = 0; index < point_count; ++index)
      points << index % 1000 << ' ' << (index / 1000) % 1000 << ' '
             << index % 17 << '\n';
  }

  NativeObjMeshKernel mesh_kernel;
  const auto mesh_start = std::chrono::steady_clock::now();
  auto mesh = mesh_kernel.import_file(mesh_path, 0.001, {}, {});
  const auto mesh_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - mesh_start);
  if (!mesh || mesh.value().statistics.triangle_count < 990'000)
    return 3;

  NativeXyzPointCloudKernel point_kernel;
  const auto point_start = std::chrono::steady_clock::now();
  auto points = point_kernel.import_file(point_path, 0.001, {}, {});
  const auto point_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - point_start);
  if (!points || points.value().statistics.point_count != point_count)
    return 4;

  MeshProcessingPipeline mesh_history;
  mesh_history.append(MeshOperation::import, "Imported million-triangle mesh");
  PointCloudProcessingPipeline point_history;
  point_history.append(PointCloudOperation::import,
                       "Imported million-point scan");
  DiscreteDocumentState state;
  state.mesh_bodies.emplace_back(MeshBodyId::generate(), "Large mesh",
                                 mesh.value(), std::move(mesh_history));
  state.point_cloud_bodies.emplace_back(PointCloudBodyId::generate(),
                                        "Large cloud", points.value(),
                                        std::move(point_history));
  const auto codec_start = std::chrono::steady_clock::now();
  auto metadata = DiscreteHistoryCodec::encode(state);
  const auto codec_time = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - codec_start);
  if (!metadata)
    return 5;

  std::cout << "dataset,elements,asset_bytes,domain_payload_bytes,import_ms,"
               "metadata_bytes,metadata_encode_us,peak_rss_bytes\n"
            << "mesh," << mesh.value().statistics.triangle_count << ','
            << std::filesystem::file_size(mesh_path) << ','
            << mesh.value().statistics.memory_bytes << ',' << mesh_time.count()
            << ',' << metadata.value().size() << ',' << codec_time.count()
            << ',' << peak_rss_bytes() << '\n'
            << "point_cloud," << points.value().statistics.point_count << ','
            << std::filesystem::file_size(point_path) << ','
            << points.value().statistics.memory_bytes << ','
            << point_time.count() << ',' << metadata.value().size() << ','
            << codec_time.count() << ',' << peak_rss_bytes() << '\n';
  std::filesystem::remove(mesh_path);
  std::filesystem::remove(point_path);
  return 0;
}

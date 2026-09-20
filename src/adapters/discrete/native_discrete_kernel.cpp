#include "native_discrete_kernel.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <numeric>
#include <span>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace duomec::adapters::discrete {
namespace {
using namespace duomec::cad::discrete;
using duomec::cad::geometry_quality::BoundingBox3d;
using duomec::cad::geometry_quality::Point3d;

core::Error import_error(std::string message,
                         const std::filesystem::path &path) {
  return {core::ErrorCode::io_failure, std::move(message), path.string()};
}
struct ContentHasher {
  std::uint64_t value{14695981039346656037ULL};
  void update(std::string_view text) {
    for (const unsigned char byte : text) {
      value ^= byte;
      value *= 1099511628211ULL;
    }
  }
  std::string finish() const {
    std::ostringstream output;
    output << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16)
           << value;
    return output.str();
  }
};

core::Result<std::string> hash_file(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    return core::Result<std::string>::failure(
        import_error("cannot hash imported asset", path));
  ContentHasher hasher;
  std::array<char, std::size_t{64} * 1024> buffer{};
  while (input) {
    input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    hasher.update(std::string_view(buffer.data(),
                                   static_cast<std::size_t>(input.gcount())));
  }
  return core::Result<std::string>::success(hasher.finish());
}

BoundingBox3d bounds(std::span<const Point3d> points) {
  if (points.empty())
    return {};
  BoundingBox3d box{points.front(), points.front(), true};
  for (const auto &point : points) {
    box.minimum.x = std::min(box.minimum.x, point.x);
    box.minimum.y = std::min(box.minimum.y, point.y);
    box.minimum.z = std::min(box.minimum.z, point.z);
    box.maximum.x = std::max(box.maximum.x, point.x);
    box.maximum.y = std::max(box.maximum.y, point.y);
    box.maximum.z = std::max(box.maximum.z, point.z);
  }
  return box;
}

struct Edge {
  std::uint32_t first{};
  std::uint32_t second{};
  auto operator==(const Edge &) const -> bool = default;
};
struct EdgeHash {
  std::size_t operator()(const Edge &edge) const noexcept {
    const std::uint64_t packed =
        (static_cast<std::uint64_t>(edge.first) << 32U) | edge.second;
    return std::hash<std::uint64_t>{}(packed);
  }
};
struct VertexBits {
  std::uint64_t x{};
  std::uint64_t y{};
  std::uint64_t z{};
  auto operator==(const VertexBits &) const -> bool = default;
};
struct VertexBitsHash {
  std::size_t operator()(const VertexBits &value) const noexcept {
    std::size_t hash = static_cast<std::size_t>(value.x);
    hash ^= static_cast<std::size_t>(value.y + 0x9e3779b97f4a7c15ULL +
                                     (hash << 6U) + (hash >> 2U));
    hash ^= static_cast<std::size_t>(value.z + 0x9e3779b97f4a7c15ULL +
                                     (hash << 6U) + (hash >> 2U));
    return hash;
  }
};

MeshStatistics calculate_mesh_statistics(const MeshBuffer &buffer) {
  MeshStatistics result;
  result.vertex_count = buffer.vertices.size();
  result.triangle_count = buffer.triangles.size();
  result.bounding_box = bounds(buffer.vertices);
  result.memory_bytes = buffer.vertices.size() * sizeof(Point3d) +
                        buffer.triangles.size() * sizeof(Triangle);
  std::unordered_set<VertexBits, VertexBitsHash> unique_vertices;
  unique_vertices.reserve(buffer.vertices.size());
  for (const auto &vertex : buffer.vertices) {
    const auto bits = [](double value) {
      return value == 0.0 ? std::uint64_t{}
                          : std::bit_cast<std::uint64_t>(value);
    };
    unique_vertices.insert({bits(vertex.x), bits(vertex.y), bits(vertex.z)});
  }
  result.duplicated_vertex_estimate =
      buffer.vertices.size() - unique_vertices.size();

  struct EdgeInfo {
    std::size_t count{};
    int orientation_sum{};
  };
  std::unordered_map<Edge, EdgeInfo, EdgeHash> edges;
  edges.reserve(buffer.triangles.size() * 2U);
  std::vector<std::uint32_t> parent(buffer.vertices.size());
  std::iota(parent.begin(), parent.end(), 0U);
  const auto root = [&parent](std::uint32_t value) {
    while (parent[value] != value) {
      parent[value] = parent[parent[value]];
      value = parent[value];
    }
    return value;
  };
  const auto unite = [&parent, &root](std::uint32_t first,
                                      std::uint32_t second) {
    first = root(first);
    second = root(second);
    if (first != second)
      parent[second] = first;
  };
  std::vector<bool> referenced(buffer.vertices.size());
  double area = 0.0;
  double signed_volume = 0.0;
  bool indices_valid = true;
  for (const auto &triangle : buffer.triangles) {
    const std::uint32_t indices[] = {triangle.first, triangle.second,
                                     triangle.third};
    if (triangle.first >= buffer.vertices.size() ||
        triangle.second >= buffer.vertices.size() ||
        triangle.third >= buffer.vertices.size()) {
      indices_valid = false;
      ++result.degenerate_triangle_count;
      continue;
    }
    for (const auto index : indices)
      referenced[index] = true;
    unite(triangle.first, triangle.second);
    unite(triangle.second, triangle.third);
    for (int index = 0; index < 3; ++index) {
      const auto from = indices[index];
      const auto to = indices[(index + 1) % 3];
      const Edge edge{std::min(from, to), std::max(from, to)};
      auto &info = edges[edge];
      ++info.count;
      info.orientation_sum += from < to ? 1 : -1;
    }
    const auto &a = buffer.vertices[triangle.first];
    const auto &b = buffer.vertices[triangle.second];
    const auto &c = buffer.vertices[triangle.third];
    const double abx = b.x - a.x;
    const double aby = b.y - a.y;
    const double abz = b.z - a.z;
    const double acx = c.x - a.x;
    const double acy = c.y - a.y;
    const double acz = c.z - a.z;
    const double cross_x = aby * acz - abz * acy;
    const double cross_y = abz * acx - abx * acz;
    const double cross_z = abx * acy - aby * acx;
    const double twice_area =
        std::sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
    if (triangle.first == triangle.second ||
        triangle.second == triangle.third || triangle.first == triangle.third ||
        twice_area == 0.0)
      ++result.degenerate_triangle_count;
    area += 0.5 * twice_area;
    signed_volume +=
        (a.x * (b.y * c.z - b.z * c.y) - a.y * (b.x * c.z - b.z * c.x) +
         a.z * (b.x * c.y - b.y * c.x)) /
        6.0;
  }
  std::unordered_set<std::uint32_t> components;
  for (std::uint32_t index = 0; index < referenced.size(); ++index)
    if (referenced[index])
      components.insert(root(index));
  result.connected_component_count = components.size();
  result.oriented = indices_valid;
  for (const auto &[edge, info] : edges) {
    (void)edge;
    if (info.count == 1)
      ++result.boundary_edge_count;
    else if (info.count > 2)
      ++result.non_manifold_edge_count;
    if (info.count == 2 && info.orientation_sum != 0)
      result.oriented = false;
  }
  result.watertight = indices_valid && result.boundary_edge_count == 0 &&
                      result.non_manifold_edge_count == 0;
  result.surface_area_si2 = area;
  if (result.watertight && result.oriented)
    result.enclosed_volume_si3 = std::abs(signed_volume);
  return result;
}

core::Result<std::uint32_t> parse_obj_index(std::string_view token,
                                            std::size_t vertex_count) {
  const auto slash = token.find('/');
  token = token.substr(0, slash);
  long long index{};
  const auto parsed =
      std::from_chars(token.data(), token.data() + token.size(), index);
  if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() ||
      index == 0)
    return core::Result<std::uint32_t>::failure({core::ErrorCode::io_failure,
                                                 "invalid OBJ vertex index",
                                                 std::string(token)});
  const long long resolved =
      index > 0 ? index - 1 : static_cast<long long>(vertex_count) + index;
  if (resolved < 0 || resolved >= static_cast<long long>(vertex_count))
    return core::Result<std::uint32_t>::failure(
        {core::ErrorCode::io_failure, "OBJ vertex index out of range",
         std::string(token)});
  if (static_cast<unsigned long long>(resolved) >
      std::numeric_limits<std::uint32_t>::max())
    return core::Result<std::uint32_t>::failure(
        {core::ErrorCode::unsupported,
         "native OBJ importer supports at most 2^32 vertices",
         std::string(token)});
  return core::Result<std::uint32_t>::success(
      static_cast<std::uint32_t>(resolved));
}

void report_progress(const ProgressCallback &progress, std::ifstream &input,
                     std::uintmax_t size, std::string_view stage) {
  if (!progress || size == 0)
    return;
  const auto position = input.tellg();
  if (position >= 0)
    progress(std::min(1.0, static_cast<double>(position) /
                               static_cast<double>(size)),
             stage);
}
} // namespace

core::Result<MeshSnapshot> NativeObjMeshKernel::import_file(
    const std::filesystem::path &path, double source_length_unit_si,
    std::stop_token cancellation, const ProgressCallback &progress) {
  if (!std::isfinite(source_length_unit_si) || source_length_unit_si <= 0.0 ||
      path.extension() != ".obj")
    return core::Result<MeshSnapshot>::failure(import_error(
        "native mesh importer requires OBJ and a positive unit scale", path));
  std::ifstream input(path);
  if (!input)
    return core::Result<MeshSnapshot>::failure(
        import_error("cannot open OBJ asset", path));
  input.imbue(std::locale::classic());
  std::error_code file_error;
  const auto file_size = std::filesystem::file_size(path, file_error);
  if (file_error)
    return core::Result<MeshSnapshot>::failure(
        import_error("cannot inspect OBJ asset size", path));
  auto buffer = std::make_shared<MeshBuffer>();
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    if ((line_number & 0x3fffU) == 0U) {
      if (cancellation.stop_requested())
        return core::Result<MeshSnapshot>::failure({core::ErrorCode::cancelled,
                                                    "OBJ import cancelled",
                                                    path.string()});
      report_progress(progress, input, file_size, "parse_obj");
    }
    std::istringstream fields(line);
    fields.imbue(std::locale::classic());
    std::string tag;
    fields >> tag;
    if (tag.empty() || tag.starts_with('#'))
      continue;
    if (tag == "v") {
      Point3d point;
      if (!(fields >> point.x >> point.y >> point.z))
        return core::Result<MeshSnapshot>::failure(import_error(
            "invalid OBJ vertex at line " + std::to_string(line_number), path));
      if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
          !std::isfinite(point.z))
        return core::Result<MeshSnapshot>::failure(import_error(
            "non-finite OBJ vertex at line " + std::to_string(line_number),
            path));
      point.x *= source_length_unit_si;
      point.y *= source_length_unit_si;
      point.z *= source_length_unit_si;
      buffer->vertices.push_back(point);
    } else if (tag == "f") {
      std::vector<std::uint32_t> polygon;
      std::string token;
      while (fields >> token) {
        auto index = parse_obj_index(token, buffer->vertices.size());
        if (!index)
          return core::Result<MeshSnapshot>::failure(index.error());
        polygon.push_back(index.value());
      }
      if (polygon.size() < 3)
        return core::Result<MeshSnapshot>::failure(
            import_error("OBJ face has fewer than three vertices at line " +
                             std::to_string(line_number),
                         path));
      for (std::size_t index = 1; index + 1 < polygon.size(); ++index)
        buffer->triangles.push_back(
            {polygon[0], polygon[index], polygon[index + 1]});
    }
  }
  if (cancellation.stop_requested())
    return core::Result<MeshSnapshot>::failure(
        {core::ErrorCode::cancelled, "OBJ import cancelled", path.string()});
  if (buffer->vertices.empty() || buffer->triangles.empty())
    return core::Result<MeshSnapshot>::failure(
        import_error("OBJ contains no triangle mesh", path));
  if (progress)
    progress(1.0, "parse_obj");
  auto asset_hash = hash_file(path);
  if (!asset_hash)
    return core::Result<MeshSnapshot>::failure(asset_hash.error());
  const std::string hash = std::move(asset_hash).value();
  MeshSnapshot snapshot;
  snapshot.revision = {1, hash, std::nullopt};
  snapshot.source_asset = {hash, path.filename(), "model/obj", file_size,
                           source_length_unit_si};
  snapshot.statistics = calculate_mesh_statistics(*buffer);
  snapshot.data = std::move(buffer);
  return core::Result<MeshSnapshot>::success(std::move(snapshot));
}

core::Result<PointCloudSnapshot> NativeXyzPointCloudKernel::import_file(
    const std::filesystem::path &path, double source_length_unit_si,
    std::stop_token cancellation, const ProgressCallback &progress) {
  if (!std::isfinite(source_length_unit_si) || source_length_unit_si <= 0.0 ||
      path.extension() != ".xyz")
    return core::Result<PointCloudSnapshot>::failure(import_error(
        "native point-cloud importer requires XYZ and a positive unit scale",
        path));
  std::ifstream input(path);
  if (!input)
    return core::Result<PointCloudSnapshot>::failure(
        import_error("cannot open XYZ asset", path));
  input.imbue(std::locale::classic());
  std::error_code file_error;
  const auto file_size = std::filesystem::file_size(path, file_error);
  if (file_error)
    return core::Result<PointCloudSnapshot>::failure(
        import_error("cannot inspect XYZ asset size", path));
  auto buffer = std::make_shared<PointCloudBuffer>();
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    if ((line_number & 0x3fffU) == 0U) {
      if (cancellation.stop_requested())
        return core::Result<PointCloudSnapshot>::failure(
            {core::ErrorCode::cancelled, "XYZ import cancelled",
             path.string()});
      report_progress(progress, input, file_size, "parse_xyz");
    }
    std::istringstream fields(line);
    fields.imbue(std::locale::classic());
    fields >> std::ws;
    if (fields.peek() == '#' || fields.eof())
      continue;
    Point3d point;
    if (!(fields >> point.x >> point.y >> point.z))
      return core::Result<PointCloudSnapshot>::failure(import_error(
          "invalid XYZ point at line " + std::to_string(line_number), path));
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        !std::isfinite(point.z))
      return core::Result<PointCloudSnapshot>::failure(import_error(
          "non-finite XYZ point at line " + std::to_string(line_number), path));
    point.x *= source_length_unit_si;
    point.y *= source_length_unit_si;
    point.z *= source_length_unit_si;
    buffer->points.push_back(point);
  }
  if (cancellation.stop_requested())
    return core::Result<PointCloudSnapshot>::failure(
        {core::ErrorCode::cancelled, "XYZ import cancelled", path.string()});
  if (buffer->points.empty())
    return core::Result<PointCloudSnapshot>::failure(
        import_error("XYZ contains no points", path));
  if (progress)
    progress(1.0, "parse_xyz");
  auto asset_hash = hash_file(path);
  if (!asset_hash)
    return core::Result<PointCloudSnapshot>::failure(asset_hash.error());
  const std::string hash = std::move(asset_hash).value();
  PointCloudSnapshot snapshot;
  snapshot.revision = {1, hash, std::nullopt};
  snapshot.source_asset = {hash, path.filename(), "text/xyz", file_size,
                           source_length_unit_si};
  snapshot.statistics.point_count = buffer->points.size();
  snapshot.statistics.bounding_box = bounds(buffer->points);
  snapshot.statistics.coordinate_precision =
      PointCloudStatistics::CoordinatePrecision::float64;
  snapshot.statistics.memory_bytes = buffer->points.size() * sizeof(Point3d);
  snapshot.data = std::move(buffer);
  return core::Result<PointCloudSnapshot>::success(std::move(snapshot));
}

} // namespace duomec::adapters::discrete

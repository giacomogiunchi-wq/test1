#pragma once
#include "duomec/core/error.hpp"
#include <chrono>
#include <filesystem>
#include <memory>
#include <stop_token>
#include <string>
#include <vector>

namespace duomec {
struct Vec3 { double x{}, y{}, z{}; };
using EntityId = std::string;
struct GeometryShape { EntityId id; };
struct Mesh { std::vector<Vec3> nodes; std::vector<unsigned> connectivity; };
struct ResultField { std::string name; std::string unit; std::vector<double> values; };
struct ProcessOptions { std::filesystem::path working_directory; std::chrono::seconds timeout{300}; };

class IGeometryKernel { public: virtual ~IGeometryKernel() = default;
  virtual core::Result<GeometryShape> make_box(Vec3 size) = 0; };
class IParametricDocument { public: virtual ~IParametricDocument() = default;
  virtual core::Result<bool> recompute() = 0; virtual core::Result<bool> save(const std::filesystem::path&) = 0; };
class ISketchSolver { public: virtual ~ISketchSolver() = default;
  virtual core::Result<bool> solve(EntityId sketch) = 0; };
class ICADImporter { public: virtual ~ICADImporter() = default;
  virtual core::Result<GeometryShape> import_file(const std::filesystem::path&) = 0; };
class ICADExporter { public: virtual ~ICADExporter() = default;
  virtual core::Result<bool> export_file(const GeometryShape&, const std::filesystem::path&) = 0; };
class IMeshGenerator { public: virtual ~IMeshGenerator() = default;
  virtual core::Result<Mesh> generate(const GeometryShape&, std::stop_token) = 0; };
class IStructuralSolver { public: virtual ~IStructuralSolver() = default;
  virtual core::Result<std::vector<ResultField>> solve(EntityId study, const ProcessOptions&, std::stop_token) = 0; };
class ICFDSolver { public: virtual ~ICFDSolver() = default;
  virtual core::Result<std::vector<ResultField>> solve(EntityId study, const ProcessOptions&, std::stop_token) = 0; };
class IMultibodySolver { public: virtual ~IMultibodySolver() = default;
  virtual core::Result<std::vector<ResultField>> solve(EntityId study, const ProcessOptions&, std::stop_token) = 0; };
class IDEMSolver { public: virtual ~IDEMSolver() = default;
  virtual core::Result<std::vector<ResultField>> solve(EntityId study, const ProcessOptions&, std::stop_token) = 0; };
class IResultReader { public: virtual ~IResultReader() = default;
  virtual core::Result<std::vector<ResultField>> read(const std::filesystem::path&) = 0; };
class IPostProcessor { public: virtual ~IPostProcessor() = default;
  virtual core::Result<bool> present(const std::vector<ResultField>&) = 0; };
}

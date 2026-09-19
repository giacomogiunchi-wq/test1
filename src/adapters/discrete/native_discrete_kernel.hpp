#pragma once

#include "duomec/cad/discrete/discrete_geometry.hpp"

namespace duomec::adapters::discrete {

class NativeObjMeshKernel final : public cad::discrete::IMeshKernel {
public:
  core::Result<cad::discrete::MeshSnapshot>
  import_file(const std::filesystem::path &path, double source_length_unit_si,
              std::stop_token cancellation,
              const cad::discrete::ProgressCallback &progress = {}) override;
};

class NativeXyzPointCloudKernel final
    : public cad::discrete::IPointCloudKernel {
public:
  core::Result<cad::discrete::PointCloudSnapshot>
  import_file(const std::filesystem::path &path, double source_length_unit_si,
              std::stop_token cancellation,
              const cad::discrete::ProgressCallback &progress = {}) override;
};

} // namespace duomec::adapters::discrete

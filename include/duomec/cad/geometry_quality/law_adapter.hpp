#pragma once

#include "duomec/cad/geometry_quality/laws.hpp"

#include <memory>
#include <string_view>

namespace duomec::cad::geometry_quality {

class IBackendScalarLaw {
public:
  virtual ~IBackendScalarLaw() = default;
  [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
};

class IScalarLawAdapter {
public:
  virtual ~IScalarLawAdapter() = default;
  [[nodiscard]] virtual core::Result<std::shared_ptr<const IBackendScalarLaw>>
  convert(const ScalarLaw &law) const = 0;
};

} // namespace duomec::cad::geometry_quality

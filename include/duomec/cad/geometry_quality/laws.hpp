#pragma once

#include "duomec/core/error.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace duomec::cad::geometry_quality {

struct LawStation {
  double parameter{};
  double value{};
  auto operator<=>(const LawStation &) const = default;
};

enum class ScalarLawKind { piecewise_linear, interpolated_cubic };

class ScalarLaw {
public:
  virtual ~ScalarLaw() = default;
  [[nodiscard]] virtual ScalarLawKind kind() const noexcept = 0;
  [[nodiscard]] virtual std::span<const LawStation>
  stations() const noexcept = 0;
  [[nodiscard]] virtual double evaluate(double parameter) const noexcept = 0;
  [[nodiscard]] virtual std::string serialize() const = 0;
};

class PiecewiseLinearLaw final : public ScalarLaw {
public:
  [[nodiscard]] static core::Result<std::shared_ptr<const PiecewiseLinearLaw>>
  create(std::vector<LawStation> stations);
  [[nodiscard]] ScalarLawKind kind() const noexcept override;
  [[nodiscard]] std::span<const LawStation> stations() const noexcept override;
  [[nodiscard]] double evaluate(double parameter) const noexcept override;
  [[nodiscard]] std::string serialize() const override;

private:
  explicit PiecewiseLinearLaw(std::vector<LawStation> stations);
  std::vector<LawStation> stations_;
};

class InterpolatedLaw final : public ScalarLaw {
public:
  [[nodiscard]] static core::Result<std::shared_ptr<const InterpolatedLaw>>
  create(std::vector<LawStation> stations);
  [[nodiscard]] ScalarLawKind kind() const noexcept override;
  [[nodiscard]] std::span<const LawStation> stations() const noexcept override;
  [[nodiscard]] double evaluate(double parameter) const noexcept override;
  [[nodiscard]] std::string serialize() const override;

private:
  InterpolatedLaw(std::vector<LawStation> stations, std::vector<double> slopes);
  std::vector<LawStation> stations_;
  std::vector<double> slopes_;
};

[[nodiscard]] core::Result<std::shared_ptr<const ScalarLaw>>
deserialize_scalar_law(std::string_view serialized);

class RadiusLaw {
public:
  [[nodiscard]] static core::Result<RadiusLaw>
  create(std::shared_ptr<const ScalarLaw> law);
  [[nodiscard]] const ScalarLaw &scalar_law() const noexcept;
  [[nodiscard]] double evaluate(double parameter) const noexcept;
  [[nodiscard]] std::string serialize() const;
  [[nodiscard]] static core::Result<RadiusLaw>
  deserialize(std::string_view serialized);

private:
  explicit RadiusLaw(std::shared_ptr<const ScalarLaw> law);
  std::shared_ptr<const ScalarLaw> law_;
};

class ScaleLaw {
public:
  [[nodiscard]] static core::Result<ScaleLaw>
  create(std::shared_ptr<const ScalarLaw> law);
  [[nodiscard]] const ScalarLaw &scalar_law() const noexcept;
  [[nodiscard]] double evaluate(double parameter) const noexcept;
  [[nodiscard]] std::string serialize() const;
  [[nodiscard]] static core::Result<ScaleLaw>
  deserialize(std::string_view serialized);

private:
  explicit ScaleLaw(std::shared_ptr<const ScalarLaw> law);
  std::shared_ptr<const ScalarLaw> law_;
};

class TwistLaw {
public:
  [[nodiscard]] static core::Result<TwistLaw>
  create(std::shared_ptr<const ScalarLaw> law);
  [[nodiscard]] const ScalarLaw &scalar_law() const noexcept;
  [[nodiscard]] double evaluate(double parameter) const noexcept;
  [[nodiscard]] std::string serialize() const;
  [[nodiscard]] static core::Result<TwistLaw>
  deserialize(std::string_view serialized);

private:
  explicit TwistLaw(std::shared_ptr<const ScalarLaw> law);
  std::shared_ptr<const ScalarLaw> law_;
};

} // namespace duomec::cad::geometry_quality

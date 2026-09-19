#include "duomec/cad/geometry_quality/laws.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <utility>

namespace duomec::cad::geometry_quality {
namespace {
constexpr std::string_view kScalarPrefix = "duomec.scalar-law/1|";

core::Result<bool> validate_stations(std::span<const LawStation> stations) {
  if (stations.size() < 2)
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument,
         "a scalar law requires at least two stations",
         {}});
  if (stations.front().parameter != 0.0 || stations.back().parameter != 1.0)
    return core::Result<bool>::failure(
        {core::ErrorCode::invalid_argument,
         "scalar law stations must include parameters 0 and 1",
         {}});
  for (std::size_t index = 0; index < stations.size(); ++index) {
    if (!std::isfinite(stations[index].parameter) ||
        !std::isfinite(stations[index].value) ||
        stations[index].parameter < 0.0 || stations[index].parameter > 1.0)
      return core::Result<bool>::failure(
          {core::ErrorCode::invalid_argument,
           "scalar law station must be finite and normalized",
           std::to_string(index)});
    if (index > 0 && stations[index].parameter <= stations[index - 1].parameter)
      return core::Result<bool>::failure(
          {core::ErrorCode::invalid_argument,
           "scalar law station parameters must be strictly increasing",
           std::to_string(index)});
  }
  return core::Result<bool>::success(true);
}

std::string encode_double(double value) {
  char buffer[64]{};
  const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value,
                                    std::chars_format::general,
                                    std::numeric_limits<double>::max_digits10);
  return std::string(buffer, result.ptr);
}

std::string serialize_law(std::string_view kind,
                          std::span<const LawStation> stations) {
  std::string output(kScalarPrefix);
  output += kind;
  output += '|';
  output += std::to_string(stations.size());
  for (const auto &station : stations) {
    output += '|';
    output += encode_double(station.parameter);
    output += ',';
    output += encode_double(station.value);
  }
  return output;
}

core::Result<double> parse_double(std::string_view text) {
  double value{};
  const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                      value, std::chars_format::general);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
      !std::isfinite(value))
    return core::Result<double>::failure({core::ErrorCode::invalid_argument,
                                          "invalid law number",
                                          std::string(text)});
  return core::Result<double>::success(value);
}

std::vector<std::string_view> split(std::string_view text, char delimiter) {
  std::vector<std::string_view> parts;
  std::size_t begin = 0;
  while (begin <= text.size()) {
    const auto end = text.find(delimiter, begin);
    parts.push_back(text.substr(begin, end == std::string_view::npos
                                           ? text.size() - begin
                                           : end - begin));
    if (end == std::string_view::npos)
      break;
    begin = end + 1;
  }
  return parts;
}

std::size_t segment_for(std::span<const LawStation> stations,
                        double parameter) {
  const auto upper =
      std::ranges::upper_bound(stations, parameter, {}, &LawStation::parameter);
  if (upper == stations.begin())
    return 0;
  if (upper == stations.end())
    return stations.size() - 2;
  return static_cast<std::size_t>(std::distance(stations.begin(), upper) - 1);
}

template <class Wrapper>
core::Result<Wrapper> deserialize_semantic(std::string_view text,
                                           std::string_view prefix) {
  if (!text.starts_with(prefix))
    return core::Result<Wrapper>::failure({core::ErrorCode::invalid_argument,
                                           "invalid semantic law prefix",
                                           std::string(text)});
  auto scalar = deserialize_scalar_law(text.substr(prefix.size()));
  if (!scalar)
    return core::Result<Wrapper>::failure(scalar.error());
  return Wrapper::create(std::move(scalar).value());
}
} // namespace

PiecewiseLinearLaw::PiecewiseLinearLaw(std::vector<LawStation> stations)
    : stations_(std::move(stations)) {}
core::Result<std::shared_ptr<const PiecewiseLinearLaw>>
PiecewiseLinearLaw::create(std::vector<LawStation> stations) {
  auto valid = validate_stations(stations);
  if (!valid)
    return core::Result<std::shared_ptr<const PiecewiseLinearLaw>>::failure(
        valid.error());
  return core::Result<std::shared_ptr<const PiecewiseLinearLaw>>::success(
      std::shared_ptr<const PiecewiseLinearLaw>(
          new PiecewiseLinearLaw(std::move(stations))));
}
ScalarLawKind PiecewiseLinearLaw::kind() const noexcept {
  return ScalarLawKind::piecewise_linear;
}
std::span<const LawStation> PiecewiseLinearLaw::stations() const noexcept {
  return stations_;
}
double PiecewiseLinearLaw::evaluate(double parameter) const noexcept {
  parameter = std::clamp(parameter, 0.0, 1.0);
  const auto index = segment_for(stations_, parameter);
  const auto &first = stations_[index];
  const auto &second = stations_[index + 1];
  const double fraction =
      (parameter - first.parameter) / (second.parameter - first.parameter);
  return std::lerp(first.value, second.value, fraction);
}
std::string PiecewiseLinearLaw::serialize() const {
  return serialize_law("linear", stations_);
}

InterpolatedLaw::InterpolatedLaw(std::vector<LawStation> stations,
                                 std::vector<double> slopes)
    : stations_(std::move(stations)), slopes_(std::move(slopes)) {}
core::Result<std::shared_ptr<const InterpolatedLaw>>
InterpolatedLaw::create(std::vector<LawStation> stations) {
  auto valid = validate_stations(stations);
  if (!valid)
    return core::Result<std::shared_ptr<const InterpolatedLaw>>::failure(
        valid.error());
  std::vector<double> secants(stations.size() - 1);
  for (std::size_t index = 0; index + 1 < stations.size(); ++index)
    secants[index] =
        (stations[index + 1].value - stations[index].value) /
        (stations[index + 1].parameter - stations[index].parameter);
  std::vector<double> slopes(stations.size());
  slopes.front() = secants.front();
  slopes.back() = secants.back();
  // Weighted harmonic slopes are deterministic and preserve monotonic station
  // intervals, preventing positive radius/scale stations from overshooting
  // through zero between stations.
  for (std::size_t index = 1; index + 1 < stations.size(); ++index) {
    if (secants[index - 1] * secants[index] <= 0.0) {
      slopes[index] = 0.0;
      continue;
    }
    const double previous_width =
        stations[index].parameter - stations[index - 1].parameter;
    const double next_width =
        stations[index + 1].parameter - stations[index].parameter;
    const double first_weight = 2.0 * next_width + previous_width;
    const double second_weight = next_width + 2.0 * previous_width;
    slopes[index] = (first_weight + second_weight) /
                    (first_weight / secants[index - 1] +
                     second_weight / secants[index]);
  }
  return core::Result<std::shared_ptr<const InterpolatedLaw>>::success(
      std::shared_ptr<const InterpolatedLaw>(
          new InterpolatedLaw(std::move(stations), std::move(slopes))));
}
ScalarLawKind InterpolatedLaw::kind() const noexcept {
  return ScalarLawKind::interpolated_cubic;
}
std::span<const LawStation> InterpolatedLaw::stations() const noexcept {
  return stations_;
}
double InterpolatedLaw::evaluate(double parameter) const noexcept {
  parameter = std::clamp(parameter, 0.0, 1.0);
  const auto index = segment_for(stations_, parameter);
  const auto &first = stations_[index];
  const auto &second = stations_[index + 1];
  const double width = second.parameter - first.parameter;
  const double value = (parameter - first.parameter) / width;
  const double value2 = value * value;
  const double value3 = value2 * value;
  const double h00 = 2.0 * value3 - 3.0 * value2 + 1.0;
  const double h10 = value3 - 2.0 * value2 + value;
  const double h01 = -2.0 * value3 + 3.0 * value2;
  const double h11 = value3 - value2;
  return h00 * first.value + h10 * width * slopes_[index] + h01 * second.value +
         h11 * width * slopes_[index + 1];
}
std::string InterpolatedLaw::serialize() const {
  return serialize_law("cubic", stations_);
}

core::Result<std::shared_ptr<const ScalarLaw>>
deserialize_scalar_law(std::string_view serialized) {
  if (!serialized.starts_with(kScalarPrefix))
    return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
        {core::ErrorCode::invalid_argument, "unsupported scalar law format",
         std::string(serialized)});
  const auto parts = split(serialized.substr(kScalarPrefix.size()), '|');
  if (parts.size() < 4)
    return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
        {core::ErrorCode::invalid_argument, "truncated scalar law", {}});
  std::size_t count{};
  const auto count_result = std::from_chars(
      parts[1].data(), parts[1].data() + parts[1].size(), count);
  if (count_result.ec != std::errc{} ||
      count_result.ptr != parts[1].data() + parts[1].size() ||
      count != parts.size() - 2)
    return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
        {core::ErrorCode::invalid_argument, "invalid scalar law station count",
         std::string(parts[1])});
  std::vector<LawStation> stations;
  stations.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    const auto values = split(parts[index + 2], ',');
    if (values.size() != 2)
      return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
          {core::ErrorCode::invalid_argument, "invalid scalar law station",
           std::string(parts[index + 2])});
    auto parameter = parse_double(values[0]);
    auto value = parse_double(values[1]);
    if (!parameter)
      return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
          parameter.error());
    if (!value)
      return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
          value.error());
    stations.push_back({parameter.value(), value.value()});
  }
  if (parts[0] == "linear") {
    auto law = PiecewiseLinearLaw::create(std::move(stations));
    if (!law)
      return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
          law.error());
    return core::Result<std::shared_ptr<const ScalarLaw>>::success(
        std::move(law).value());
  }
  if (parts[0] == "cubic") {
    auto law = InterpolatedLaw::create(std::move(stations));
    if (!law)
      return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
          law.error());
    return core::Result<std::shared_ptr<const ScalarLaw>>::success(
        std::move(law).value());
  }
  return core::Result<std::shared_ptr<const ScalarLaw>>::failure(
      {core::ErrorCode::unsupported, "unknown scalar law kind",
       std::string(parts[0])});
}

RadiusLaw::RadiusLaw(std::shared_ptr<const ScalarLaw> law)
    : law_(std::move(law)) {}
core::Result<RadiusLaw>
RadiusLaw::create(std::shared_ptr<const ScalarLaw> law) {
  if (!law ||
      std::ranges::any_of(law->stations(), [](const LawStation &station) {
        return station.value <= 0.0;
      }))
    return core::Result<RadiusLaw>::failure(
        {core::ErrorCode::invalid_argument,
         "radius law station values must be positive SI lengths",
         {}});
  return core::Result<RadiusLaw>::success(RadiusLaw(std::move(law)));
}
const ScalarLaw &RadiusLaw::scalar_law() const noexcept { return *law_; }
double RadiusLaw::evaluate(double parameter) const noexcept {
  return law_->evaluate(parameter);
}
std::string RadiusLaw::serialize() const {
  return "duomec.radius-law/1|" + law_->serialize();
}
core::Result<RadiusLaw> RadiusLaw::deserialize(std::string_view serialized) {
  return deserialize_semantic<RadiusLaw>(serialized, "duomec.radius-law/1|");
}

ScaleLaw::ScaleLaw(std::shared_ptr<const ScalarLaw> law)
    : law_(std::move(law)) {}
core::Result<ScaleLaw> ScaleLaw::create(std::shared_ptr<const ScalarLaw> law) {
  if (!law ||
      std::ranges::any_of(law->stations(), [](const LawStation &station) {
        return station.value <= 0.0;
      }))
    return core::Result<ScaleLaw>::failure(
        {core::ErrorCode::invalid_argument,
         "scale law station values must be positive ratios",
         {}});
  return core::Result<ScaleLaw>::success(ScaleLaw(std::move(law)));
}
const ScalarLaw &ScaleLaw::scalar_law() const noexcept { return *law_; }
double ScaleLaw::evaluate(double parameter) const noexcept {
  return law_->evaluate(parameter);
}
std::string ScaleLaw::serialize() const {
  return "duomec.scale-law/1|" + law_->serialize();
}
core::Result<ScaleLaw> ScaleLaw::deserialize(std::string_view serialized) {
  return deserialize_semantic<ScaleLaw>(serialized, "duomec.scale-law/1|");
}

TwistLaw::TwistLaw(std::shared_ptr<const ScalarLaw> law)
    : law_(std::move(law)) {}
core::Result<TwistLaw> TwistLaw::create(std::shared_ptr<const ScalarLaw> law) {
  if (!law)
    return core::Result<TwistLaw>::failure(
        {core::ErrorCode::invalid_argument, "twist law is required", {}});
  return core::Result<TwistLaw>::success(TwistLaw(std::move(law)));
}
const ScalarLaw &TwistLaw::scalar_law() const noexcept { return *law_; }
double TwistLaw::evaluate(double parameter) const noexcept {
  return law_->evaluate(parameter);
}
std::string TwistLaw::serialize() const {
  return "duomec.twist-law/1|" + law_->serialize();
}
core::Result<TwistLaw> TwistLaw::deserialize(std::string_view serialized) {
  return deserialize_semantic<TwistLaw>(serialized, "duomec.twist-law/1|");
}

} // namespace duomec::cad::geometry_quality

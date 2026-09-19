#pragma once
namespace duomec::core {
enum class LengthUnit { metre, millimetre, inch };
[[nodiscard]] double to_si(double value, LengthUnit unit) noexcept;
[[nodiscard]] double from_si(double metres, LengthUnit unit) noexcept;
}

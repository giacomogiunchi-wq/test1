#include "duomec/core/units.hpp"
namespace duomec::core { namespace { double scale(LengthUnit u) { switch(u) { case LengthUnit::metre:return 1.; case LengthUnit::millimetre:return .001; case LengthUnit::inch:return .0254; } return 1.; } }
double to_si(double value, LengthUnit unit) noexcept { return value * scale(unit); }
double from_si(double metres, LengthUnit unit) noexcept { return metres / scale(unit); }
}

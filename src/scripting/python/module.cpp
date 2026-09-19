#include "duomec/core/units.hpp"
#include <pybind11/pybind11.h>
PYBIND11_MODULE(_duomec, module) { module.doc() = "Duomec Platform core bindings"; module.def("millimetres_to_metres", [](double value) { return duomec::core::to_si(value, duomec::core::LengthUnit::millimetre); }); }

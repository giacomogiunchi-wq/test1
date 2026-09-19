cmake_minimum_required(VERSION 3.25)
project(DuomecPlatform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(DUOMEC_BUILD_DESKTOP "Build the Qt/OCCT desktop application" OFF)
option(DUOMEC_BUILD_TESTS "Build tests" ON)
option(DUOMEC_BUILD_PYTHON "Build Python bindings" OFF)
option(DUOMEC_ENABLE_GMSH "Enable the Gmsh adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_CODE_ASTER "Enable the Code_Aster adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_OPENFOAM "Enable the OpenFOAM adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_CHRONO "Enable the Project Chrono adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_VTK "Enable VTK post-processing (future milestone)" OFF)
option(DUOMEC_ENABLE_DEM "Enable the experimental LAMMPS DEM adapter" OFF)

add_library(duomec_core
  src/core/command.cpp
  src/core/log.cpp
  src/core/units.cpp)
add_library(Duomec::Core ALIAS duomec_core)
target_include_directories(duomec_core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(duomec_core PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(duomec_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(duomec_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

if(DUOMEC_BUILD_DESKTOP)
  find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
  find_package(OpenCASCADE 8.0 REQUIRED)
  qt_standard_project_setup()
  qt_add_executable(duomec
    src/ui/qt/main.cpp src/ui/qt/main_window.cpp src/ui/qt/main_window.hpp
    src/ui/qt/occt_viewport.cpp src/ui/qt/occt_viewport.hpp)
  target_include_directories(duomec PRIVATE ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(duomec PRIVATE Duomec::Core Qt6::Core Qt6::Gui Qt6::Widgets
    Qt6::OpenGLWidgets ${OpenCASCADE_LIBRARIES})
endif()

if(DUOMEC_BUILD_PYTHON)
  find_package(pybind11 2.13.6 CONFIG REQUIRED)
  pybind11_add_module(_duomec src/scripting/python/module.cpp)
  target_link_libraries(_duomec PRIVATE Duomec::Core)
endif()

include(CTest)
if(BUILD_TESTING AND DUOMEC_BUILD_TESTS)
  add_executable(duomec_unit_tests tests/unit/core_tests.cpp)
  target_link_libraries(duomec_unit_tests PRIVATE Duomec::Core)
  add_test(NAME duomec.unit COMMAND duomec_unit_tests)
endif()

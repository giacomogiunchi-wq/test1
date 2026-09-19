cmake_minimum_required(VERSION 3.25)
project(DuomecPlatform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(DUOMEC_BUILD_DESKTOP "Build the Qt/OCCT desktop application" OFF)
option(DUOMEC_BUILD_TESTS "Build tests" ON)
option(DUOMEC_BUILD_PYTHON "Build Python bindings" OFF)
option(DUOMEC_ENABLE_OCAF "Build the Milestone 1A OCAF persistence adapter" OFF)
option(DUOMEC_ENABLE_GMSH "Enable the Gmsh adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_CODE_ASTER "Enable the Code_Aster adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_OPENFOAM "Enable the OpenFOAM adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_CHRONO "Enable the Project Chrono adapter (future milestone)" OFF)
option(DUOMEC_ENABLE_VTK "Enable VTK post-processing (future milestone)" OFF)
option(DUOMEC_ENABLE_DEM "Enable the experimental LAMMPS DEM adapter" OFF)

function(duomec_enable_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
  endif()
endfunction()

add_library(duomec_core
  src/core/command.cpp
  src/core/log.cpp
  src/core/units.cpp)
add_library(Duomec::Core ALIAS duomec_core)
target_include_directories(duomec_core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(duomec_core PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_core)

add_library(duomec_cad_document
  src/cad/document/ids.cpp
  src/cad/document/memory_document_store.cpp
  src/cad/document/parametric_document.cpp)
add_library(Duomec::CadDocument ALIAS duomec_cad_document)
target_include_directories(duomec_cad_document PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_cad_document PUBLIC Duomec::Core)
target_compile_features(duomec_cad_document PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_cad_document)

add_library(duomec_cad_features
  src/cad/features/feature_framework.cpp)
add_library(Duomec::CadFeatures ALIAS duomec_cad_features)
target_include_directories(duomec_cad_features PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_cad_features PUBLIC Duomec::CadDocument)
target_compile_features(duomec_cad_features PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_cad_features)

if(DUOMEC_ENABLE_OCAF)
  find_package(OpenCASCADE 8.0 REQUIRED)
  add_library(duomec_ocaf_adapter src/adapters/ocaf/ocaf_document_store.cpp)
  add_library(Duomec::OcafAdapter ALIAS duomec_ocaf_adapter)
  target_include_directories(duomec_ocaf_adapter
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/adapters/ocaf>
    PRIVATE ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(duomec_ocaf_adapter PUBLIC Duomec::CadDocument PRIVATE ${OpenCASCADE_LIBRARIES})
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
  add_executable(duomec_document_tests tests/unit/document_tests.cpp)
  target_link_libraries(duomec_document_tests PRIVATE Duomec::CadDocument)
  add_test(NAME duomec.document.unit COMMAND duomec_document_tests)
  add_executable(duomec_feature_framework_tests tests/unit/feature_framework_tests.cpp)
  target_link_libraries(duomec_feature_framework_tests PRIVATE Duomec::CadFeatures)
  add_test(NAME duomec.feature_framework.unit COMMAND duomec_feature_framework_tests)
  add_executable(duomec_feature_framework_benchmark tests/benchmarks/feature_framework_benchmark.cpp)
  target_link_libraries(duomec_feature_framework_benchmark PRIVATE Duomec::CadFeatures)
  if(DUOMEC_ENABLE_OCAF)
    add_executable(duomec_ocaf_integration_tests tests/integration/ocaf_document_tests.cpp)
    target_link_libraries(duomec_ocaf_integration_tests PRIVATE Duomec::OcafAdapter)
    add_test(NAME duomec.ocaf.integration COMMAND duomec_ocaf_integration_tests)
  endif()
endif()

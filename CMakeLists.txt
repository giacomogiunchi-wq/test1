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

add_library(duomec_runtime_instrumentation
  src/runtime/performance.cpp
  src/runtime/benchmark_corpus.cpp)
add_library(Duomec::RuntimeInstrumentation ALIAS duomec_runtime_instrumentation)
target_include_directories(duomec_runtime_instrumentation PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_runtime_instrumentation PUBLIC Duomec::Core)
target_compile_features(duomec_runtime_instrumentation PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_runtime_instrumentation)

add_library(duomec_cad_document
  src/cad/document/ids.cpp
  src/cad/document/memory_document_store.cpp
  src/cad/document/parametric_document.cpp)
add_library(Duomec::CadDocument ALIAS duomec_cad_document)
target_include_directories(duomec_cad_document PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_cad_document PUBLIC Duomec::Core)
target_compile_features(duomec_cad_document PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_cad_document)

add_library(duomec_assembly_relations src/assembly/relations.cpp)
add_library(Duomec::AssemblyRelations ALIAS duomec_assembly_relations)
target_include_directories(duomec_assembly_relations PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_assembly_relations PUBLIC Duomec::CadDocument)
target_compile_features(duomec_assembly_relations PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_assembly_relations)

add_library(duomec_cad_features
  src/cad/features/feature_framework.cpp)
add_library(Duomec::CadFeatures ALIAS duomec_cad_features)
target_include_directories(duomec_cad_features PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_cad_features PUBLIC Duomec::CadDocument)
target_compile_features(duomec_cad_features PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_cad_features)

add_library(duomec_geometry_quality
  src/cad/geometry_quality/laws.cpp
  src/cad/geometry_quality/quality.cpp)
add_library(Duomec::GeometryQuality ALIAS duomec_geometry_quality)
target_include_directories(duomec_geometry_quality PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_geometry_quality PUBLIC Duomec::Core)
target_compile_features(duomec_geometry_quality PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_geometry_quality)

add_library(duomec_discrete_geometry
  src/cad/discrete/discrete_geometry.cpp
  src/cad/discrete/discrete_codec.cpp)
add_library(Duomec::DiscreteGeometry ALIAS duomec_discrete_geometry)
target_include_directories(duomec_discrete_geometry PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(duomec_discrete_geometry PUBLIC Duomec::CadDocument Duomec::GeometryQuality)
target_compile_features(duomec_discrete_geometry PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_discrete_geometry)

add_library(duomec_native_discrete_adapter
  src/adapters/discrete/native_discrete_kernel.cpp)
add_library(Duomec::NativeDiscreteAdapter ALIAS duomec_native_discrete_adapter)
target_include_directories(duomec_native_discrete_adapter
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/adapters/discrete>)
target_link_libraries(duomec_native_discrete_adapter PUBLIC Duomec::DiscreteGeometry)
target_compile_features(duomec_native_discrete_adapter PUBLIC cxx_std_20)
duomec_enable_warnings(duomec_native_discrete_adapter)

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
  add_executable(duomec_geometry_quality_tests tests/unit/geometry_quality_tests.cpp)
  target_link_libraries(duomec_geometry_quality_tests PRIVATE Duomec::GeometryQuality)
  add_test(NAME duomec.geometry_quality.unit COMMAND duomec_geometry_quality_tests)
  add_executable(duomec_geometry_quality_benchmark tests/benchmarks/geometry_quality_benchmark.cpp)
  target_link_libraries(duomec_geometry_quality_benchmark PRIVATE Duomec::GeometryQuality)
  add_executable(duomec_discrete_geometry_tests tests/unit/discrete_geometry_tests.cpp)
  target_compile_definitions(duomec_discrete_geometry_tests PRIVATE DUOMEC_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(duomec_discrete_geometry_tests PRIVATE Duomec::NativeDiscreteAdapter)
  add_test(NAME duomec.discrete_geometry.unit COMMAND duomec_discrete_geometry_tests)
  add_executable(duomec_runtime_instrumentation_tests tests/unit/runtime_instrumentation_tests.cpp)
  target_link_libraries(duomec_runtime_instrumentation_tests PRIVATE Duomec::RuntimeInstrumentation)
  add_test(NAME duomec.runtime_instrumentation.unit COMMAND duomec_runtime_instrumentation_tests)
  add_executable(duomec_assembly_relations_tests tests/unit/assembly_relations_tests.cpp)
  target_link_libraries(duomec_assembly_relations_tests PRIVATE Duomec::AssemblyRelations)
  add_test(NAME duomec.assembly_relations.unit COMMAND duomec_assembly_relations_tests)
  add_executable(duomec_large_assembly_baseline tests/benchmarks/large_assembly_baseline.cpp)
  target_link_libraries(duomec_large_assembly_baseline PRIVATE Duomec::RuntimeInstrumentation)
  add_executable(duomec_discrete_geometry_benchmark tests/benchmarks/discrete_geometry_benchmark.cpp)
  target_link_libraries(duomec_discrete_geometry_benchmark PRIVATE Duomec::NativeDiscreteAdapter)
  if(DUOMEC_ENABLE_OCAF)
    add_executable(duomec_ocaf_integration_tests tests/integration/ocaf_document_tests.cpp)
    target_link_libraries(duomec_ocaf_integration_tests PRIVATE Duomec::OcafAdapter)
    add_test(NAME duomec.ocaf.integration COMMAND duomec_ocaf_integration_tests)
  endif()
endif()

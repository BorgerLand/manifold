# Copyright 2022 The Manifold Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6.1
)
FetchContent_MakeAvailable(Corrosion)

if(MANIFOLD_DEBUG AND MANIFOLD_ASSERT)
  set(MESHBOOL_FEATURES "test_thoroughly")
else()
  set(MESHBOOL_FEATURES "test")
endif()

corrosion_import_crate(
  MANIFEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Cargo.toml"
  FEATURES "${MESHBOOL_FEATURES}"
)

if(MESHBOOL_FEATURES STREQUAL "test_thoroughly")
  corrosion_add_target_rustflags(meshbool "-C" "debug-assertions=yes")
endif()

set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/../../generated/test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../generated/test.h"
  PROPERTIES GENERATED TRUE
)

target_sources(
  meshbool
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../generated/test.cpp"
)

add_library(manifold ALIAS meshbool)

target_link_libraries(meshbool INTERFACE $<$<BOOL:${TRACY_ENABLE}>:TracyClient>)

target_link_libraries(
  meshbool
  INTERFACE
    # optional dependencies
    $<$<BOOL:${MANIFOLD_PAR}>:TBB::tbb>
)

# make sure users use appropriate c++ standard when including our headers
target_compile_features(meshbool INTERFACE cxx_std_17)
set(
  OPTIONS
  MANIFOLD_DEBUG
  MANIFOLD_ASSERT
  MANIFOLD_TIMING
  MANIFOLD_NO_IOSTREAM
  TRACY_ENABLE
  TRACY_MEMORY_USAGE
)
foreach(OPT IN LISTS OPTIONS)
  if(${${OPT}})
    target_compile_definitions(meshbool INTERFACE ${OPT})
  endif()
endforeach()
if(MANIFOLD_PAR)
  target_compile_definitions(meshbool INTERFACE MANIFOLD_PAR=1)
else()
  target_compile_definitions(meshbool INTERFACE MANIFOLD_PAR=-1)
endif()

# MANIFOLD_NO_IOSTREAM also defines MANIFOLD_NO_FILESYSTEM (the two
# track together in practice — `<filesystem>` paths are only useful
# if you can `<fstream>` them, which needs iostream). Source code
# uses the macros separately for semantic clarity (NO_IOSTREAM gates
# the iostream-using public API; NO_FILESYSTEM gates filesystem use
# in test_main.cpp's fixture helpers + main()).
if(MANIFOLD_NO_IOSTREAM)
  target_compile_definitions(meshbool INTERFACE MANIFOLD_NO_FILESYSTEM)
endif()

target_include_directories(
  meshbool
  INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../generated>
)

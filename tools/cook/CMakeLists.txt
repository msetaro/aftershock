cmake_minimum_required(VERSION 3.25)
project(aftershock_cook LANGUAGES C CXX)
add_executable(aftershock-cook-bc7 bc7.cpp ../../third_party/bc7enc/bc7enc.cpp)
target_include_directories(aftershock-cook-bc7 PRIVATE ../../third_party/bc7enc)
target_compile_features(aftershock-cook-bc7 PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(aftershock-cook-bc7 PRIVATE /fp:strict)
else()
  target_compile_options(aftershock-cook-bc7 PRIVATE -ffp-contract=off -fno-fast-math)
endif()

include(../../cmake/Sources.cmake)
list(TRANSFORM OGG_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/../../")
list(TRANSFORM VORBIS_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/../../")
add_executable(aftershock-cook-audio audio.cpp ${OGG_SOURCES} ${VORBIS_SOURCES})
target_include_directories(aftershock-cook-audio PRIVATE
  ../../third_party/libogg/include ../../third_party/libvorbis/include ../../third_party/libvorbis/lib)
target_compile_features(aftershock-cook-audio PRIVATE cxx_std_20)
if(NOT MSVC)
  target_link_libraries(aftershock-cook-audio PRIVATE m)
  target_compile_options(aftershock-cook-audio PRIVATE -ffp-contract=off -fno-fast-math)
endif()

add_executable(aftershock-cook-lod lod.cpp
  ../../third_party/meshoptimizer/src/allocator.cpp
  ../../third_party/meshoptimizer/src/simplifier.cpp)
target_compile_features(aftershock-cook-lod PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(aftershock-cook-lod PRIVATE /fp:strict)
else()
  target_compile_options(aftershock-cook-lod PRIVATE -ffp-contract=off -fno-fast-math)
endif()

list(TRANSFORM RECAST_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/../../")
list(TRANSFORM DETOUR_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/../../")
add_executable(aftershock-cook-navigation navigation.cpp ${RECAST_SOURCES} ${DETOUR_SOURCES}
  ../../engine/qcommon/cm_load.cpp ../../engine/qcommon/cm_patch.cpp
  ../../engine/qcommon/cm_polylib.cpp ../../engine/qcommon/cm_physics.cpp
  ../../engine/qcommon/cm_test.cpp ../../engine/qcommon/q_math.cpp
  ../../engine/qcommon/q_shared.cpp ../../engine/qcommon/md4.cpp
  ../../engine/qcommon/state.cpp ../../third_party/sha256/sha-256.c)
target_include_directories(aftershock-cook-navigation PRIVATE
  ../../third_party/recast/Recast/Include ../../third_party/recast/Detour/Include)
target_compile_features(aftershock-cook-navigation PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(aftershock-cook-navigation PRIVATE /fp:strict "$<$<COMPILE_LANGUAGE:CXX>:/GR-;/EHs-c->")
else()
  target_compile_options(aftershock-cook-navigation PRIVATE -ffp-contract=off -fno-fast-math "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>")
endif()

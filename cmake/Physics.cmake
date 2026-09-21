# Keep dependency options inside this function's directory scope.
function(aftershock_add_physics)
  if(TARGET joltc)
    return()
  endif()
  get_filename_component(physics_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
  set(JOLT_PHYSICS_ROOT "${physics_root}/third_party/jolt")
  if(NOT EXISTS "${JOLT_PHYSICS_ROOT}/Build/CMakeLists.txt")
    message(FATAL_ERROR "The pinned Jolt source is missing; builds never fetch dependencies")
  endif()
  foreach(option_name JPH_MASTER_PROJECT JPH_SAMPLES JPH_TESTS JPH_INSTALL ENABLE_INSTALL
      JPH_BUILD_SHARED BUILD_SHARED_LIBS OVERRIDE_CXX_FLAGS INTERPROCEDURAL_OPTIMIZATION
      FLOATING_POINT_EXCEPTIONS_ENABLED CPP_EXCEPTIONS_ENABLED CPP_RTTI_ENABLED
      DOUBLE_PRECISION DISABLE_CUSTOM_ALLOCATOR USE_STD_VECTOR
      JPH_USE_DX12 JPH_USE_VK JPH_USE_MTL JPH_USE_CPU_COMPUTE
      DEBUG_RENDERER_IN_DEBUG_AND_RELEASE DEBUG_RENDERER_IN_DISTRIBUTION
      PROFILER_IN_DEBUG_AND_RELEASE PROFILER_IN_DISTRIBUTION
      USE_SSE4_1 USE_SSE4_2 USE_AVX USE_AVX2 USE_AVX512 USE_LZCNT USE_TZCNT USE_F16C USE_FMADD)
    set(${option_name} OFF)
  endforeach()
  set(CROSS_PLATFORM_DETERMINISTIC ON)
  add_subdirectory("${physics_root}/third_party/joltc" "${CMAKE_CURRENT_BINARY_DIR}/joltc")
  set_property(TARGET Jolt joltc PROPERTY CXX_STANDARD 20)
endfunction()

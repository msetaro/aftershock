# Offline artifact cache: unchanged sources reuse the verified committed SPIR-V.
# A cache miss requires the pinned compiler; the executable never compiles shaders.
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
set(SHADER_COMPILER "" CACHE FILEPATH "Pinned glslang executable for offline shader compilation")
set(shader_command "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/shaders/build.py"
    --output "${CMAKE_BINARY_DIR}/shaders")
if(SHADER_COMPILER)
  list(APPEND shader_command --compiler "${SHADER_COMPILER}")
endif()
# Compilation databases used by lifetime/tidy checks need the generated header too.
execute_process(COMMAND ${shader_command} COMMAND_ERROR_IS_FATAL ANY)
add_custom_target(shader_package
  COMMAND ${shader_command}
  BYPRODUCTS "${CMAKE_BINARY_DIR}/shaders/shader_package.h"
             "${CMAKE_BINARY_DIR}/shaders/shader_package.json"
  VERBATIM)
add_custom_target(shaders
  COMMAND ${shader_command} --compile
  COMMENT "Compile the pinned offline shader package"
  VERBATIM)
add_dependencies(shaders shader_package)
add_dependencies(vulkan shader_package)
target_include_directories(vulkan PRIVATE "${CMAKE_BINARY_DIR}/shaders")

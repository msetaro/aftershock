# Vendor options remain local to this dependency's directory scope.
function(aftershock_add_audio)
  if(TARGET opus)
    return()
  endif()
  get_filename_component(audio_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
  foreach(option_name BUILD_SHARED_LIBS BUILD_TESTING OPUS_BUILD_SHARED_LIBRARY
      OPUS_BUILD_FRAMEWORK OPUS_BUILD_TESTING OPUS_BUILD_PROGRAMS OPUS_CUSTOM_MODES
      OPUS_DRED OPUS_OSCE OPUS_DEEP_PLC OPUS_NONTHREADSAFE_PSEUDOSTACK
      OPUS_INSTALL_PKG_CONFIG_MODULE OPUS_INSTALL_CMAKE_CONFIG_MODULE)
    set(${option_name} OFF)
  endforeach()
  set(OPUS_STATIC_RUNTIME ON)
  # Keep a portable scalar codec on all targets; SIMD tuning needs its own measurements.
  set(OPUS_DISABLE_INTRINSICS ON)
  add_subdirectory("${audio_root}/third_party/opus" "${CMAKE_CURRENT_BINARY_DIR}/opus" EXCLUDE_FROM_ALL)
  set_property(TARGET opus PROPERTY C_STANDARD 99)
endfunction()

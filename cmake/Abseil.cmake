macro(p4c_obtain_abseil)
  p4c_dependency_compat(P4C_USE_PREINSTALLED_ABSEIL absl)
  set(P4C_ABSEIL_VERSION "20240722.1")
  if(ENABLE_ABSEIL_STATIC)
    set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()

  p4c_find_package(
    NAME absl
    URL https://github.com/abseil/abseil-cpp/releases/download/${P4C_ABSEIL_VERSION}/abseil-cpp-${P4C_ABSEIL_VERSION}.tar.gz
    URL_HASH SHA256=40cee67604060a7c8794d931538cb55f4d444073e556980c88b6c49bb9b19bb7
    FIND_PACKAGE_ARGUMENTS "CONFIG ${P4C_absl_FIND_ARGUMENTS}"
    EXCLUDE_FROM_ALL YES
    SYSTEM YES
    OPTIONS
      "CMAKE_UNITY_BUILD OFF"
      "ABSL_USE_EXTERNAL_GOOGLETEST ON"
      "ABSL_FIND_GOOGLETEST OFF"
      "ABSL_BUILD_TESTING OFF"
      "ABSL_ENABLE_INSTALL OFF"
      "ABSL_USE_SYSTEM_INCLUDES ON"
      "ABSL_PROPAGATE_CXX_STD ON"
  )
  if(ENABLE_ABSEIL_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
  endif()

  if(absl_ADDED)
    # SYSTEM suppresses warnings in consumers, not in Abseil's own compilation.
    get_all_targets(ABSL_BUILD_TARGETS ${absl_SOURCE_DIR})
    foreach(target ${ABSL_BUILD_TARGETS})
      get_target_property(target_type ${target} TYPE)
      if(target MATCHES "absl_.*" AND NOT target_type STREQUAL "INTERFACE_LIBRARY")
        target_compile_options(${target} PRIVATE "-Wno-error" "-w")
      endif()
    endforeach()
  endif()
  message(STATUS "Done with setting up Abseil for P4C.")
endmacro()

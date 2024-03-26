macro(p4c_obtain_abseil)
  option(
    P4C_USE_PREINSTALLED_ABSEIL
    "Look for a preinstalled version of Abseil in the system instead of installing the library using FetchContent."
    OFF
  )

  # If P4C_USE_PREINSTALLED_ABSEIL is ON just try to find a preinstalled version of Abseil.
  if(P4C_USE_PREINSTALLED_ABSEIL)
    if(ENABLE_ABSEIL_STATIC)
      set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif()

    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    find_package(absl REQUIRED)

    if(ENABLE_ABSEIL_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
  else()
    set(P4C_ABSEIL_VERSION "20240116.1")
    message("Fetching Abseil version ${P4C_ABSEIL_VERSION} for P4C...")

    # Unity builds do not work for Abseil...
    set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)

    # Print out download state while setting up Abseil.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)

    set(ABSL_USE_EXTERNAL_GOOGLETEST ON)
    set(ABSL_FIND_GOOGLETEST OFF)
    set(ABSL_BUILD_TESTING OFF)
    set(ABSL_ENABLE_INSTALL OFF)
    set(ABSL_USE_SYSTEM_INCLUDES ON)
    set(ABSL_PROPAGATE_CXX_STD ON)
    
    FetchContent_Declare(
      abseil
      URL https://github.com/abseil/abseil-cpp/releases/download/${P4C_ABSEIL_VERSION}/abseil-cpp-${P4C_ABSEIL_VERSION}.tar.gz
      URL_HASH SHA256=3c743204df78366ad2eaf236d6631d83f6bc928d1705dd0000b872e53b73dc6a
      USES_TERMINAL_DOWNLOAD TRUE
      GIT_PROGRESS TRUE
    )
    fetchcontent_makeavailable_but_exclude_install(abseil)

    # Suppress warnings for all Abseil targets.
    get_all_targets(ABSL_BUILD_TARGETS ${absl_SOURCE_DIR})
    foreach(target in ${ABSL_BUILD_TARGETS})
      if(target MATCHES "absl_*")
        # Do not suppress warnings for Abseil library targets that are aliased.
        get_target_property(target_type ${target} TYPE)
        if (NOT ${target_type} STREQUAL "INTERFACE_LIBRARY")
          set_target_properties(${target} PROPERTIES COMPILE_FLAGS "-Wno-error -w")
        endif()
      endif()
    endforeach()

    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  endif()

  message("Done with setting up Abseil for P4C.")
endmacro(p4c_obtain_abseil)

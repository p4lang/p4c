# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Boost.Graph and Tofino need more Boost headers than the compiler core. Keep
# those include paths confined to the backends and use a consistent release:
# older Graph releases pull in Math headers incompatible with standalone MP.
function(p4c_obtain_backend_boost)
  option(
    P4C_USE_PREINSTALLED_BOOST
    "Use installed Boost for the Graphs and Tofino backends instead of FetchContent."
    OFF
  )
  if(P4C_USE_PREINSTALLED_BOOST)
    find_package(Boost 1.83 REQUIRED COMPONENTS graph)
    add_library(p4c_backend_boost_headers INTERFACE)
    target_include_directories(p4c_backend_boost_headers SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
  else()
    message(STATUS "Fetching Boost version 1.90.0 for the Graphs and Tofino backends...")
    set(FETCHCONTENT_QUIET OFF)
    set(CMAKE_UNITY_BUILD OFF)
    set(BUILD_TESTING OFF)
    set(BUILD_SHARED_LIBS OFF)
    set(BOOST_INCLUDE_LIBRARIES graph)
    # Config was already obtained with standalone Multiprecision.
    if(TARGET Boost::config)
      set(BOOST_EXCLUDE_LIBRARIES config)
    endif()
    set(BOOST_SKIP_INSTALL_RULES ON)
    FetchContent_Declare(
      backend_boost
      URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
      URL_HASH SHA256=aca59f889f0f32028ad88ba6764582b63c916ce5f77b31289ad19421a96c555f
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    fetchcontent_makeavailable_but_exclude_install(backend_boost)

    # Keep P4C's warning policy from turning third-party warnings into errors.
    get_all_targets(_boost_targets ${backend_boost_SOURCE_DIR})
    foreach(target ${_boost_targets})
      get_target_property(_boost_target_type ${target} TYPE)
      if(NOT _boost_target_type STREQUAL "INTERFACE_LIBRARY")
        target_compile_options(${target} PRIVATE "-Wno-error" "-w")
      endif()
    endforeach()

    # Both backends use headers from compiled Boost libraries too. Expose their
    # headers without introducing unnecessary link dependencies.
    file(GLOB _boost_header_dirs "${backend_boost_SOURCE_DIR}/libs/*/include"
      "${backend_boost_SOURCE_DIR}/libs/numeric/*/include")
    add_library(p4c_backend_boost_headers INTERFACE)
    target_include_directories(p4c_backend_boost_headers SYSTEM INTERFACE ${_boost_header_dirs})
  endif()
  message(STATUS "Done with setting up backend Boost for P4C.")
endfunction()

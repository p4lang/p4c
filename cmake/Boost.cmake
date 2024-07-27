macro(p4c_obtain_boost)
  option(
    P4C_USE_PREINSTALLED_BOOST
    "Look for a preinstalled version of Boost in the system instead of installing the library using FetchContent."
    OFF
  )

  set(P4C_BOOST_VERSION "1.85.0")

  # If P4C_USE_PREINSTALLED_BOOST is ON just try to find a preinstalled version of Boost.
  if(P4C_USE_PREINSTALLED_BOOST)
    if(NOT BUILD_SHARED_LIBS)
      # Link Boost statically
      # See https://cmake.org/cmake/help/latest/module/FindBoost.html for details
      set(Boost_USE_STATIC_LIBS ON)
      set(Boost_USE_STATIC_RUNTIME OFF)
    endif()

    # The boost graph headers are optional and only required by the graphs back end.
    find_package (Boost QUIET COMPONENTS graph)
    if (Boost_FOUND)
      set (HAVE_LIBBOOST_GRAPH 1)
    else ()
      message (WARNING "Boost graph headers not found, will not build 'graphs' backend")
    endif ()
    find_package (Boost REQUIRED COMPONENTS format multiprecision)
    include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIRS})

  else()
    message(STATUS "Fetching Boost version ${P4C_BOOST_VERSION} for P4C...")
    # Print out download state while setting up Boost.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    # Unity builds do not work for Abseil...
    set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)

    # Add boost library sources.
    # format, multiprecision, and iostreams are needed by P4C core.
    set(BOOST_INCLUDE_LIBRARIES format multiprecision iostreams)
    option(P4C_BOOST_LIBRARIES
      "Build and make these boost modules available. Modules are separated with semicolons."
      "")
    set(BOOST_ENABLE_CMAKE ON)

    if (ENABLE_P4C_GRAPHS)
      set(HAVE_LIBBOOST_GRAPH 1)
      list(APPEND BOOST_INCLUDE_LIBRARIES graph)
    endif()
    list(APPEND BOOST_INCLUDE_LIBRARIES ${P4C_BOOST_LIBRARIES})

    # Always link Boost statically.
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    # Download and extract the boost library from GitHub.
    message(STATUS "Downloading and extracting boost library sources. This may take some time...")
    FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-${P4C_BOOST_VERSION}/boost-${P4C_BOOST_VERSION}-cmake.tar.xz
        USES_TERMINAL_DOWNLOAD TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(Boost)

    # Force inclusion of the correct Boost headers. This is necessary because users may have their own system boost headers which can be in conflict.
    set (Boost_INCLUDE_DIRS ${Boost_BINARY_DIR}/include)
    file(GLOB __boost_include_directories
        ${Boost_SOURCE_DIR}/libs/**/include ${Boost_SOURCE_DIR}/libs/**/**/include
    )
    foreach(__boost_include_directory ${__boost_include_directories})
      if(IS_DIRECTORY ${__boost_include_directory})
        include_directories(BEFORE SYSTEM ${__boost_include_directory})
      endif()
    endforeach()
    # Suppress warnings for all Boost targets.
    get_all_targets(BOOST_BUILD_TARGETS ${Boost_SOURCE_DIR})
    foreach(target in ${BOOST_BUILD_TARGETS})
      if(target MATCHES "boost_*")
        # Do not suppress warnings for Boost library targets that are aliased.
        get_target_property(target_type ${target} TYPE)
        if (NOT ${target_type} STREQUAL "INTERFACE_LIBRARY")
          target_compile_options(${target} PRIVATE "-Wno-error" "-w")
        endif()
      endif()
    endforeach()


    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
  endif()

  set(HAVE_LIBBOOST_IOSTREAMS TRUE)
  set (P4C_BOOST_LIBRARIES Boost::iostreams Boost::format Boost::multiprecision)
  list(APPEND P4C_LIB_DEPS ${P4C_BOOST_LIBRARIES})

  message(STATUS "Done with setting up Boost for P4C.")
endmacro(p4c_obtain_boost)

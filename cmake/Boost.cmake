macro(p4c_obtain_boost)
  option(
    P4C_USE_PREINSTALLED_BOOST
    "Look for a preinstalled version of Boost in the system instead of installing the library using FetchContent."
    OFF
  )

  # If P4C_USE_PREINSTALLED_BOOST is ON just try to find a preinstalled version of Boost.
  if(P4C_USE_PREINSTALLED_BOOST)
    if(NOT BUILD_SHARED_LIBS)
      # Link Boost statically.
      # See https://cmake.org/cmake/help/latest/module/FindBoost.html for details.
      set(Boost_USE_STATIC_LIBS ON)
      set(Boost_USE_STATIC_RUNTIME OFF)
    endif()
    find_package(Boost REQUIRED COMPONENTS iostreams)
    list(APPEND P4C_BOOST_LIBRARIES Boost::iostreams)
  else()
    set(P4C_BOOST_VERSION "1.86.0")
    set(P4C_BOOST_HASH "2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57")
    message(STATUS "Fetching Boost version ${P4C_BOOST_VERSION} for P4C...")
    # Print out download state while setting up Boost.
    set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
    set(FETCHCONTENT_QUIET OFF)
    # Unity builds do not work for Boost...
    set(CMAKE_UNITY_BUILD_PREV ${CMAKE_UNITY_BUILD})
    set(CMAKE_UNITY_BUILD OFF)

    # Add boost modules.
    # format, multiprecision, and iostreams are needed by P4C core.
    set(BOOST_INCLUDE_LIBRARIES format multiprecision iostreams)
    list(APPEND BOOST_INCLUDE_LIBRARIES ${ADDITIONAL_P4C_BOOST_MODULES})
    set(BOOST_ENABLE_CMAKE ON)

    # Always link local Boost statically.
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    # Download and extract Boost from GitHub.
    message(STATUS "Downloading and extracting boost library sources. This may take some time...")
    FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-${P4C_BOOST_VERSION}/boost-${P4C_BOOST_VERSION}-cmake.tar.xz
        URL_HASH SHA256=${P4C_BOOST_HASH}
        USES_TERMINAL_DOWNLOAD TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(Boost)

    # Suppress warnings for all Boost targets.
    get_all_targets(BOOST_BUILD_TARGETS ${Boost_SOURCE_DIR})
    set(boost_target_includes "")
    foreach(target in ${BOOST_BUILD_TARGETS})
      if(target MATCHES "boost_*")
        # Do not suppress warnings for Boost library targets that are aliased.
        get_target_property(target_type ${target} TYPE)
        if(NOT ${target_type} STREQUAL "INTERFACE_LIBRARY")
          target_compile_options(${target} PRIVATE "-Wno-error" "-w")
        endif()
        # Force inclusion of the Boost headers as system headers.
        # This is necessary because users may have their own system boost headers
        # which can be in conflict. Also ensures that linters don't complain.
        get_target_property(target_includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
        set_target_properties(${target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${target_includes}")
      endif()
    endforeach()
    # Reset temporary variable modifications.
    set(CMAKE_UNITY_BUILD ${CMAKE_UNITY_BUILD_PREV})
    set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})
    list(APPEND P4C_BOOST_LIBRARIES Boost::iostreams Boost::format Boost::multiprecision)
  endif()

  include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIRS})
  set(HAVE_LIBBOOST_IOSTREAMS TRUE)
  list(APPEND P4C_LIB_DEPS ${P4C_BOOST_LIBRARIES})

  message(STATUS "Done with setting up Boost for P4C.")
endmacro(p4c_obtain_boost)

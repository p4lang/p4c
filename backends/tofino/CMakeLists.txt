# ###### Barefoot Compilers Toolchain
MESSAGE("-- Adding p4c-barefoot")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/spdlog.cmake)
include(${P4C_SOURCE_DIR}/cmake/Z3.cmake)

obtain_z3()

set (BFN_P4C_SOURCE_DIR ${PROJECT_SOURCE_DIR}/backends/tofino)

OPTION(ENABLE_STATIC_LIBS OFF)
OPTION(ENABLE_BAREFOOT_INTERNAL "Enable Barefoot Internal" ON)
OPTION(ENABLE_ASSERTIONS "Enable assertions" ON)

# BUILD_STATIC_BFP4C_LIBS should never be used directly, see BUILD_STATIC_BFP4C_LIBS_INTERNAL below
OPTION(BUILD_STATIC_BFP4C_LIBS OFF "Build static libs (archives) of bf-p4c components \
                              (tofinobackend, bfn_p4runtime) for external usage. If not set, the \
                              libraries are build only as object and cannot be used easily by \
                              other projects compiled alongside. Building static libraries slows \
                              down compilation.")

if (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
  add_definitions("-DRELEASE_BUILD=1")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if (ENABLE_STATIC_LIBS)
  # Look for static libraries when using find_library
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  # Force linking libstdc++ statically. Note that dependencies may bring in
  # dynamic versions of it, so that's why we look above for .a
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-z,muldefs")

  # Link Boost statically
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
endif()


if (ENABLE_ASSERTIONS)
  # NDEBUG is automatically defined on non-Debug builds by CMake.
  # To enable assertions, we need to explicitly undefine NDEBUG.
  if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions( -UNDEBUG )
  endif()
endif()

if (ENABLE_TESTING)
  enable_testing()
endif()

set(BOOST_MIN_VERSION "1.58.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED)

set(INTERFACES_DIR ${BFN_P4C_SOURCE_DIR}/compiler_interfaces)

# Get the version from bf-p4c/version.h
file (STRINGS "${BFN_P4C_SOURCE_DIR}/bf-p4c/version.h"
  __version
  REGEX "#define BF_P4C_VERSION.*"
  )
string (REGEX MATCH ".*BF_P4C_VERSION.*([0-9]+)\\.([0-9]+)\\.([0-9]+)([-0-9a-z\\.]*).*"
  __bfn_p4c_version ${__version})
set (BFN_P4C_VERSION_MAJOR ${CMAKE_MATCH_1})
set (BFN_P4C_VERSION_MINOR ${CMAKE_MATCH_2})
set (BFN_P4C_VERSION_PATCH ${CMAKE_MATCH_3})
set (BFN_P4C_VERSION_RC ${CMAKE_MATCH_4})
set (BFN_P4C_VERSION
  "${BFN_P4C_VERSION_MAJOR}.${BFN_P4C_VERSION_MINOR}.${BFN_P4C_VERSION_PATCH}")
if (BFN_P4C_VERSION_RC)
  set (BFN_P4C_VERSION "${BFN_P4C_VERSION}${BFN_P4C_VERSION_RC}")
endif()
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  execute_process (COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE BFN_P4C_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE rc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else()
  if(NOT DEFINED ENV{BFN_P4C_GIT_SHA})
    set (BFN_P4C_GIT_SHA "Unknown")
  else()
    set (BFN_P4C_GIT_SHA $ENV{BFN_P4C_GIT_SHA})
  endif()
endif()
MESSAGE(STATUS "p4c-barefoot version: ${BFN_P4C_VERSION}")

# Generate the sha specific version file. It includes the GIT SHA.
# Because this version changes frequently, we include it separately from the normal version files.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_sha_version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/bf-p4c/git_sha_version.h" @ONLY)


function(get_schema_version SCHEMA_FILE OUTPUT_VAR)
    set(SCHEMA_PATH "${INTERFACES_DIR}/schemas/${SCHEMA_FILE}.py")
    if(NOT EXISTS "${SCHEMA_PATH}")
        message(SEND_ERROR "Schema file not found: ${SCHEMA_PATH}")
    endif()

    file(READ "${SCHEMA_PATH}" file_content)

    string(REGEX MATCH "major_version[ \t]*=[ \t]*([0-9]+)" _ "${file_content}")
    set(major_version "${CMAKE_MATCH_1}")
    string(REGEX MATCH "minor_version[ \t]*=[ \t]*([0-9]+)" _ "${file_content}")
    set(minor_version "${CMAKE_MATCH_1}")
    string(REGEX MATCH "patch_version[ \t]*=[ \t]*([0-9]+)" _ "${file_content}")
    set(patch_version "${CMAKE_MATCH_1}")
    if(NOT DEFINED major_version OR major_version STREQUAL ""
       OR NOT DEFINED minor_version OR minor_version STREQUAL ""
       OR NOT DEFINED patch_version OR patch_version STREQUAL "")
        message(SEND_ERROR "Failed to parse version from ${VERSION_FILE}")
    endif()

    set(${OUTPUT_VAR} "${major_version}.${minor_version}.${patch_version}" PARENT_SCOPE)
endfunction()

if (ENABLE_WERROR)
  # fixme: we do want implicit-fallthrough to be an error, but we have to
  # supress in in p4c and in generated code somehow
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-error=implicit-fallthrough")
endif()

# # Gtest needs additional includes, add them here.
# if (ENABLE_GTESTS)
#   target_include_directories(gtestp4c
#     PRIVATE "${BFN_P4C_SOURCE_DIR}/p4c/test/frameworks/gtest/googlemock/include"
#     PRIVATE "${BFN_P4C_SOURCE_DIR}/p4c/test/frameworks/gtest/googletest/include"
#     PRIVATE "${P4C_BINARY_DIR}/extensions/bf-p4c/logging"
#     PRIVATE "bf-p4c/logging"
#     PRIVATE "${P4C_BINARY_DIR}/control-plane/"
#   )
#   # -Wdeprecated-copy warns in gtest, so out of our control
#   # TODO: -Wno-unused-function warns in p4c, should be possible to fix it
#   target_compile_options(gtestp4c PUBLIC -Wno-unused-function -Wno-deprecated-copy)
# endif()

add_custom_target(ctags-all
  COMMAND cd ${BFN_P4C_SOURCE_DIR}/bf-p4c &&
          ctags -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp
                ${P4C_SOURCE_DIR}/frontends ${P4C_SOURCE_DIR}/ir
                ${P4C_SOURCE_DIR}/midend/ ${P4C_SOURCE_DIR}/lib .
  DEPENDS ctags-asm ctags
  COMMENT "Building ctags")

if (ENABLE_DOXYGEN)
  find_package(Doxygen QUIET COMPONENTS dot)
  if(DOXYGEN_FOUND)
    set (doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set (doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
      DEPENDS genIR  # Generate ir-generated.h
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building Doxygen documentation"
      VERBATIM)

    add_custom_target(doc-install
      # Place a symlink to the generated documentation to the /var/www/html
      # directory use cmake's create_symlink which must exist always and
      # overrides the target (`ls -sf` creates a link as bf-p4c-compiles/html if
      # bf-p4c-compiles already exists)
      COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/html /var/www/html/bf-p4c-compilers
      COMMENT "Installing Doxygen documentation"
      VERBATIM)
  else()
    message(WARNING "Doxygen and dot are needed to build the documentation.")
  endif()
endif()

set (CPACK_GENERATOR "TBZ2")
set (CPACK_PACKAGE_NAME "p4c")
set (CPACK_PACKAGE_VERSION_MAJOR ${BFN_P4C_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${BFN_P4C_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${BFN_P4C_VERSION_PATCH})
set (CPACK_PACKAGE_VERSION "${BFN_P4C_VERSION}")
set (CPACK_PACKAGE_CONTACT "Barefoot Networks, Inc. <p4c@barefootnetworks.com>")
set (CPACK_PACKAGE_VENDOR "Barefoot Networks, Inc.")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "P4C compilers for P4_14 and P4_16")
set (CPACK_PACKAGE_DESCRIPTION "P4C compilers for the Barefoot Networks Tofino architecture")
set (CPACK_PROJECT_CONFIG_FILE "${BFN_P4C_SOURCE_DIR}/CPackOptions.cmake")
set (CPACK_STRIP_FILES TRUE)

include (CPack)

set (BF_P4C_IR_DEF_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/tofino.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/arch.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/mau.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/parde.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/parde-lowered.def
  )

# publish IR_DEF_FILES upstream
set (IR_DEF_FILES ${IR_DEF_FILES} ${BF_P4C_IR_DEF_FILES} PARENT_SCOPE)

set (BF_P4C_IR_SRCS
  bf-p4c/midend/path_linearizer.cpp
  bf-p4c/ir/dbprint-tofino.cpp
  bf-p4c/ir/bitrange.cpp
  bf-p4c/ir/control_flow_visitor.cpp
  bf-p4c/ir/ir_enums.cpp
  bf-p4c/ir/gateway_control_flow.cpp
  bf-p4c/ir/mau.cpp
  bf-p4c/ir/thread_visitor.cpp
  bf-p4c/ir/tofino.cpp
  bf-p4c/ir/tofino_write_context.cpp
  bf-p4c/ir/unique_id.cpp
  bf-p4c/mau/hash_function.cpp
  bf-p4c/parde/marshal.cpp
  bf-p4c/parde/match_register.cpp
  bf-p4c/parde/clot/clot.cpp
  bf-p4c/phv/phv.cpp
  # FIXME: This should be a library.
  bf-utils/dynamic_hash/dynamic_hash.cpp
  bf-utils/dynamic_hash/bfn_hash_algorithm.cpp
  )

set(QUAL_BF_P4C_IR_SRCS)
foreach(src IN LISTS BF_P4C_IR_SRCS)
  set(QUAL_BF_P4C_IR_SRCS ${QUAL_BF_P4C_IR_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
set(EXTENSION_IR_SOURCES ${EXTENSION_IR_SOURCES} ${QUAL_BF_P4C_IR_SRCS} PARENT_SCOPE)
set(EXTENSION_IR_LIBS ${EXTENSION_IR_LIBS} tofinospecs PARENT_SCOPE)


add_subdirectory(bf-p4c)

# Initialize bf-asm after bf-p4c.
add_subdirectory(bf-asm)

set (XFAIL_TESTS
)
set (TOFINO_TEST_SUITES
  "${P4C_SOURCE_DIR}/testdata/p4_14_samples/*.p4"
)
set (TOFINO_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-tofino-test.sh)
p4c_add_tests("tofino" ${TOFINO_TEST_DRIVER} "${TOFINO_TEST_SUITES}" "${XFAIL_TESTS}")


# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 3.0.2 FATAL_ERROR)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    MESSAGE(STATUS "Enabling ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project (P4C)

set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set (P4C_RUNTIME_OUTPUT_DIRECTORY bin)
set (P4C_LIBRARY_OUTPUT_DIRECTORY lib)
set (P4C_ARTIFACTS_OUTPUT_DIRECTORY share/p4c)
set (CMAKE_USE_RELATIVE_PATHS 1)

OPTION (ENABLE_DOCS "Build the documentation" OFF)
OPTION (ENABLE_GTESTS "Enable building and running GTest unit tests" ON)
OPTION (ENABLE_BMV2 "Build the BMV2 backend (required for the full test suite)" ON)
OPTION (ENABLE_EBPF "Build the EBPF backend (required for the full test suite)" ON)
OPTION (ENABLE_P4TEST "Build the P4Test backend (required for the full test suite)" ON)
OPTION (ENABLE_P4C_GRAPHS "Build the p4c-graphs backend" ON)
OPTION (ENABLE_PROTOBUF_STATIC "Link against Protobuf statically" ON)
OPTION (ENABLE_GC "Use libgc" ON)
OPTION (ENABLE_MULTITHREAD "Use multithreading" OFF)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "RELEASE")
endif()

if (NOT $ENV{P4C_VERSION} STREQUAL "")
  # Allow the version to be set from outside
  set (P4C_VERSION $ENV{P4C_VERSION})
else()
  # Semantic version numbering: <major>.<minor>.<patch>[-rcX]
  # Examples: 0.5.1, 1.0.0-rc1, 1.0.1-alpha
  set (P4C_SEM_VERSION_STRING "1.1.0-rc1")
  string (REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)([-0-9a-z\\.]*).*"
    __p4c_version ${P4C_SEM_VERSION_STRING})
  set (P4C_VERSION_MAJOR ${CMAKE_MATCH_1})
  set (P4C_VERSION_MINOR ${CMAKE_MATCH_2})
  set (P4C_VERSION_PATCH ${CMAKE_MATCH_3})
  set (P4C_VERSION_RC ${CMAKE_MATCH_4})
  execute_process (COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE P4C_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE rc
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  set (P4C_VERSION "${P4C_SEM_VERSION_STRING} (SHA: ${P4C_GIT_SHA})")
endif()
include(P4CUtils)
include(UnifiedBuild)

# # search in /usr/local first
# set (CMAKE_FIND_ROOT_PATH "/usr/local/bin" "${CMAKE_FIND_ROOT_PATH}")
set (P4C_CXX_FLAGS "")
set (P4C_LIB_DEPS)

# required tools and libraries
find_package (PythonInterp REQUIRED)
find_package (FLEX REQUIRED)
find_package (BISON 3.0.2 REQUIRED)
if (ENABLE_PROTOBUF_STATIC)
  set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif ()
find_package (Protobuf 3.0.0 REQUIRED)
if (ENABLE_PROTOBUF_STATIC)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
endif ()
# The boost graph headers are optional and only required by the graphs backend
find_package (Boost QUIET COMPONENTS graph)
if (Boost_FOUND)
  set (HAVE_LIBBOOST_GRAPH 1)
else ()
  message (WARNING "Boost graph headers not found, will not build 'graphs' backend")
endif ()
find_package (Boost REQUIRED COMPONENTS iostreams)
# otherwise ordered_map code tries to use boost::get (graph)
add_definitions ("-DBOOST_NO_ARGUMENT_DEPENDENT_LOOKUP")
if (ENABLE_GC)
  find_package (LibGc 7.2.0 REQUIRED)
  set (HAVE_LIBGC 1)
endif ()
if (ENABLE_MULTITHREAD)
  add_definitions(-DMULTITHREAD)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set (P4C_LIB_DEPS "${P4C_LIB_DEPS};${CMAKE_THREAD_LIBS_INIT}")
endif()
find_package (LibGmp REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${LIBGMP_INCLUDE_DIR})
include_directories(${LIBGC_INCLUDE_DIR})
set (HAVE_LIBBOOST_IOSTREAMS 1)
set (HAVE_LIBGMP 1)
set (HAVE_LIBGMPXX 1)
set (P4C_LIB_DEPS "${P4C_LIB_DEPS};${Boost_LIBRARIES};${LIBGMP_LIBRARIES};${LIBGC_LIBRARIES}")

# other required libraries
p4c_add_library (rt clock_gettime HAVE_CLOCK_GETTIME)

# check includes
include (CheckIncludeFile)
check_include_file (execinfo.h HAVE_EXECINFO_H)
check_include_file (ucontext.h HAVE_UCONTEXT_H)
include (CheckIncludeFileCXX)
check_include_file_cxx (cxxabi.h HAVE_CXXABI_H)

# check functions

# set libraries to be used with check_function_exists
set (CMAKE_REQUIRED_LIBRARIES_PRECHECK ${CMAKE_REQUIRED_LIBRARIES})
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${LIBGC_LIBRARIES})

include (CheckFunctionExists)
check_function_exists (memchr HAVE_MEMCHR)
check_function_exists (pipe2 HAVE_PIPE2)
check_function_exists (GC_print_stats HAVE_GC_PRINT_STATS)

# restore CMAKE_REQUIRED_LIBRARIES
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_PRECHECK})

# python modules
include (FindPythonModule)
find_python_module (difflib REQUIRED)
find_python_module (shutil REQUIRED)
find_python_module (tempfile REQUIRED)
find_python_module (subprocess REQUIRED)
find_python_module (re REQUIRED)

# other packages
find_package (Doxygen)
find_package (BMV2)

# enable CTest
enable_testing ()


# if we want to manage versions in CMake ...
# include (cmake/P4CVersion.cmake)
# set (CPACK_PACKAGE_VERSION_MAJOR ${__P4C_VERSION_MAJOR})
# set (CPACK_PACKAGE_VERSION_MINOR ${__P4C_VERSION_MINOR})
# set (CPACK_PACKAGE_VERSION_PATCH ${__P4C_VERSION_PATCH})
# if (__P4C_VERSION_RC)
#   set (CPACK_PACKAGE_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH}-${__P4C_VERSION_RC})
# endif ()

# set (CMAKE_CXX_EXTENSIONS OFF) # prefer using -std=c++11 rather than -std=gnu++11
# CMAKE_CXX_STANDARD was introduced in CMake 3.1, so for older versions of CMake, we use -std=gnu++11
if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
else ()
    set (CMAKE_CXX_STANDARD 11)
endif ()
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_cxx_compiler_option ("-Wall")
add_cxx_compiler_option ("-Wextra")
add_cxx_compiler_option ("-Wno-overloaded-virtual")
add_cxx_compiler_option ("-Wno-deprecated")

# If we're on GCC, use the gold linker if available.
if(CMAKE_COMPILER_IS_GNUCC)
    execute_process(
            COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version
            ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    if ("${LD_VERSION}" MATCHES "GNU gold")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=gold")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
        message(STATUS "Using the GNU gold linker.")
    else ()
        message(STATUS "Using the default system linker.")
    endif ()
    unset(LD_VERSION)
endif ()

include_directories (
  ${P4C_SOURCE_DIR}/frontends
  ${P4C_SOURCE_DIR}/backends
  ${P4C_SOURCE_DIR}/extensions
  ${P4C_SOURCE_DIR}
  ${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest/include
  ${P4C_BINARY_DIR}
  )
add_definitions (-DCONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}")
add_definitions (-DCONFIG_PKGDATADIR="${CMAKE_INSTALL_PREFIX}/${P4C_ARTIFACTS_OUTPUT_DIRECTORY}")

set (CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${P4C_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${P4C_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${P4C_CXX_FLAGS}")
set (CPPLINT_FILES)           # list to collect all files that need lint
set (TEST_TAGS "p4" CACHE INTERNAL "test tags") # list to collect all test tags
set (IR_DEF_FILES)           # list to collect all .def files

# Other configuration files that need to be generated
configure_file ("${P4C_SOURCE_DIR}/cmake/config.h.cmake" "${P4C_BINARY_DIR}/config.h")

set (IR_GENERATED_SRCS
  ${P4C_BINARY_DIR}/ir/ir-generated.h
  ${P4C_BINARY_DIR}/ir/ir-generated.cpp
  ${P4C_BINARY_DIR}/ir/gen-tree-macro.h)
# IR_GENERATOR_DIRECTORY is used to set the RUNTIME_OUTPUT_DIRECTORY property
# of the irgenerator target to the matching path
set (IR_GENERATOR_DIRECTORY ${P4C_BINARY_DIR}/tools/ir-generator)
set (IR_GENERATOR ${IR_GENERATOR_DIRECTORY}/irgenerator)

# the order of adding subdirectories matters because of target dependencies
add_subdirectory (tools/driver)
add_subdirectory (lib)
add_subdirectory (tools/ir-generator)
add_subdirectory (ir)

# component libraries: must be defined before being used in the
# backend executables
set (P4C_LIBRARIES controlplane midend frontend ir p4ctoolkit)

# add extensions - before the frontends as they produce IR and extra frontend sources
set(EXTENSION_FRONTEND_SOURCES)
# extra sources that need to be linked directly into p4test so that
# extensions can provide specific conversions (e.g., for externs)
set(EXTENSION_P4_14_CONV_SOURCES)

file (GLOB p4c_extensions RELATIVE ${P4C_SOURCE_DIR}/extensions ${P4C_SOURCE_DIR}/extensions/*)
MESSAGE ("-- Available extensions ${p4c_extensions}")
foreach (ext ${p4c_extensions})
  if (EXISTS ${P4C_SOURCE_DIR}/extensions/${ext}/CMakeLists.txt)
    # Generate an option that makes it possible to disable this extension.
    string(MAKE_C_IDENTIFIER ${ext} EXT_AS_IDENTIFIER)
    string(TOUPPER ${EXT_AS_IDENTIFIER} EXT_AS_OPTION_NAME)
    string(CONCAT ENABLE_EXT_OPTION "ENABLE_" ${EXT_AS_OPTION_NAME})
    string(CONCAT EXT_HELP_TEXT "Build the " ${ext} " backend")
    OPTION (${ENABLE_EXT_OPTION} ${EXT_HELP_TEXT} ON)

    if (${ENABLE_EXT_OPTION})
        add_subdirectory (extensions/${ext})
    endif()
  endif()
endforeach(ext)

add_subdirectory (frontends)
add_subdirectory (midend)
add_subdirectory (control-plane)

if (ENABLE_BMV2)
    add_subdirectory (backends/bmv2)
endif ()
if (ENABLE_EBPF)
    add_subdirectory (backends/ebpf)
endif ()
if (ENABLE_P4TEST)
    add_subdirectory (backends/p4test)
endif ()
if (ENABLE_P4C_GRAPHS AND HAVE_LIBBOOST_GRAPH EQUAL 1)
  add_subdirectory (backends/graphs)
endif ()
if (ENABLE_GTESTS)
  add_subdirectory (test)
endif ()

# IR Generation
set_source_files_properties(${IR_GENERATOR} PROPERTIES GENERATED TRUE)

# Fixup #line directives in the generated IR files
# This is a vestige of automake. CMake handles dependencies correctly,
# so we should output the line directives directly from the generator
#
# Moreover, we generate these files to a temporary location and update the
# build files only if they have changed. This avoids rebuilding the whole
# tree when small changes to the .def files affect only the .cpp file and
# not the headers.
set (fixup_file "${CMAKE_CURRENT_BINARY_DIR}/irgen-fixup.awk")
set_source_files_properties (${fixup_file} PROPERTIES GENERATED TRUE)
file(WRITE "${fixup_file}" "/^#\$/ { printf \"#line %d \\\"${CMAKE_CURRENT_BINARY_DIR}/ir/%s\\\"\\n\", NR+1, name; next; } 1\n")
set(temp_ir_genfiles
  ir/ir-generated.cpp.tmp ir/ir-generated.cpp.fixup
  ir/ir-generated.h.tmp   ir/ir-generated.h.fixup
  ir/gen-tree-macro.h.tmp ir/gen-tree-macro.h.fixup
  )
set_source_files_properties (${temp_ir_genfiles} PROPERTIES GENERATED TRUE)

add_custom_command (OUTPUT ${IR_GENERATED_SRCS}
  COMMAND ${IR_GENERATOR} -i ir/ir-generated.cpp.tmp -o ir/ir-generated.h.tmp -t ir/gen-tree-macro.h.tmp ${IR_DEF_FILES}
  COMMAND awk -v name=ir-generated.cpp -f ${fixup_file} ir/ir-generated.cpp.tmp > ir/ir-generated.cpp.fixup
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ir/ir-generated.cpp.fixup ir/ir-generated.cpp
  COMMAND awk -v name=ir-generated.h   -f ${fixup_file} ir/ir-generated.h.tmp > ir/ir-generated.h.fixup
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ir/ir-generated.h.fixup ir/ir-generated.h
  COMMAND awk -v name=gen-tree-macro.h -f ${fixup_file} ir/gen-tree-macro.h.tmp > ir/gen-tree-macro.h.fixup
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ir/gen-tree-macro.h.fixup ir/gen-tree-macro.h
  MAIN_DEPENDENCY ${IR_GENERATOR}
  DEPENDS irgenerator ${IR_DEF_FILES}
  COMMENT "Generating IR class files")

add_custom_target(genIR DEPENDS ${IR_GENERATED_SRCS})

# Header files
add_custom_target(update_includes ALL
  #FIXME -- should only run this when headers change -- how to accomplish that?
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${P4C_SOURCE_DIR}/p4include ${P4C_BINARY_DIR}/p4include
)

# Installation
# Targets install themselves. Here we install the core headers
install (DIRECTORY ${P4C_SOURCE_DIR}/p4include
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}
  FILES_MATCHING PATTERN "*.p4")

# cpplint
list ( SORT CPPLINT_FILES )
set (CPPLINT_CMD ${P4C_SOURCE_DIR}/tools/cpplint.py)
set (CPPLINT_ARGS --root=${P4C_SOURCE_DIR} --extensions=h,hpp,cpp,ypp,l)
add_custom_target(cpplint
  COMMAND ${CPPLINT_CMD} ${CPPLINT_ARGS} ${CPPLINT_FILES}
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "cpplint")
add_custom_target(cpplint-quiet
  COMMAND ${CPPLINT_CMD} --quiet ${CPPLINT_ARGS} ${CPPLINT_FILES}
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "cpplint quietly")
# add cpplint as a test so that make check can proceed in parallel
add_test(NAME cpplint
  COMMAND ${CPPLINT_CMD} --quiet ${CPPLINT_ARGS} ${CPPLINT_FILES}
  WORKING_DIRECTORY ${P4C_SOURCE_DIR})
set_tests_properties(cpplint PROPERTIES LABELS cpplint)

# tags, etags
set (CTAGS_DIRS backends extensions frontends ir lib tools midend)
add_custom_target(tags
  COMMAND ctags -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp -I abstract=class -I interface=class ${CTAGS_DIRS}
  COMMAND cd tools/ir-generator && ctags -R --langmap=Flex:+.l,YACC:+.ypp . ../../lib
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "Generating ctags")
add_custom_target(etags
  COMMAND ctags -e -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp -I abstract=class -I interface=class ${CTAGS_DIRS}
  COMMAND cd tools/ir-generator && ctags -e -R --langmap=Flex:+.l,YACC:+.ypp . ../../lib
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "Generating extended ctags")

# check, recheck, check-*, check-ifail, gtest
p4c_get_nprocs(__parallel_test)
MESSAGE(STATUS "CTest parallel: -j ${__parallel_test}")
set (P4C_XFAIL_LOG ${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/LastXfail.txt)
set (P4C_TEST_FLAGS -j "${__parallel_test}")

p4c_add_make_check(all)
list (REMOVE_DUPLICATES TEST_TAGS)
foreach(t ${TEST_TAGS})
  p4c_add_make_check(${t})
endforeach()

add_custom_target(check
  DEPENDS check-all)

add_custom_target(recheck
  DEPENDS recheck-all)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Uninstall.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# docs
if (ENABLE_DOCS AND DOXYGEN_FOUND)
  if(DOXYGEN_DOT_FOUND)
    set (HAVE_DOT 'YES')
  else()
    set (HAVE_DOT 'NO')
  endif()
  set (DOXYGEN_FILE ${P4C_SOURCE_DIR}/docs/doxygen/doxygen.cfg)
  add_custom_target(docs ALL
    COMMAND export SRCDIR="${P4C_SOURCE_DIR}" &&
            export PROJECT=${PROJECT_NAME} &&
            export HAVE_DOT=${HAVE_DOT} &&
            export DOT_PATH=${DOXYGEN_DOT_PATH} &&
            export GENERATE_HTML='YES' &&
            export GENERATE_PDF='NO' &&
            export DOCDIR=${P4C_BINARY_DIR}/doxygen_out &&
            ${DOXYGEN_EXECUTABLE} ${DOXYGEN_FILE}
    DEPENDS genIR
    COMMENT "Generating documentation")
  install (DIRECTORY ${P4C_BINARY_DIR}/doxygen-out/html/
    DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/docs)
endif()

# \TODO: packaging

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

project (P4C)

set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set (P4C_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
set (P4C_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib)
set (P4C_ARTIFACTS_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/share/p4c)
set (CMAKE_USE_RELATIVE_PATHS 1)

OPTION (ENABLE_DOCS "Build the documentation" OFF)
OPTION (ENABLE_P4RUNTIME_TO_PD "Build the P4Runtime PD Generator" OFF)

# # search in /usr/local first
# set (CMAKE_FIND_ROOT_PATH "/usr/local/bin" "${CMAKE_FIND_ROOT_PATH}")
set (P4C_CXX_FLAGS "")
set (P4C_LIB_DEPS)

# required tools and libraries
find_package (PythonInterp REQUIRED)
find_package (FLEX REQUIRED)
find_package (BISON REQUIRED)
find_package(Protobuf 3.0.0 REQUIRED)
find_package (Boost REQUIRED COMPONENTS iostreams)
if (Boost_IOSTREAMS_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  set (HAVE_LIBBOOST_IOSTREAMS 1)
  set (P4C_LIB_DEPS "${P4C_LIB_DEPS};${Boost_LIBRARIES}")
endif()
# other required libraries
include (CheckLibraryExists)
check_library_exists (gc GC_malloc "" HAVE_LIBGC)
if (HAVE_LIBGC)
  set (P4C_LIB_DEPS "${P4C_LIB_DEPS};gc")
endif()
check_library_exists (gmp __gmpz_init "" HAVE_LIBGMP)
if (HAVE_LIBGMP)
  set (P4C_LIB_DEPS "${P4C_LIB_DEPS};gmp")
endif()
check_library_exists (rt clock_gettime "" HAVE_CLOCK_GETTIME)
if (HAVE_CLOCK_GETTIME)
  set (P4C_LIB_DEPS "${P4C_LIB_DEPS};rt")
endif()
find_library (HAVE_LIBGMPXX gmpxx PATHS /usr/local/lib)
if (HAVE_LIBGMPXX)
  set (P4C_LIB_DEPS "${P4C_LIB_DEPS};gmpxx")
endif()
find_package_handle_standard_args (libgmpxx DEFAULT_MSG HAVE_LIBGMPXX)

# check includes
include (CheckIncludeFile)
check_include_file (execinfo.h HAVE_EXECINFO_H)
check_include_file (ucontext.h HAVE_UCONTEXT_H)

# check functions
include (CheckFunctionExists)
check_function_exists (memchr HAVE_MEMCHR)
check_function_exists (pipe2 HAVE_PIPE2)

# python modules
include (FindPythonModule)
find_python_module (difflib REQUIRED)
find_python_module (shutil REQUIRED)
find_python_module (tempfile REQUIRED)
find_python_module (subprocess REQUIRED)
find_python_module (re REQUIRED)
if (ENABLE_P4RUNTIME_TO_PD)
  find_python_module (tenjin REQUIRED)
endif ()

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
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include(P4CUtils)

add_cxx_compiler_option ("-Wall")
add_cxx_compiler_option ("-Wextra")
add_cxx_compiler_option ("-Wno-overloaded-virtual")
add_cxx_compiler_option ("-Wno-deprecated")

include_directories (
  ${P4C_SOURCE_DIR}/frontends
  ${P4C_SOURCE_DIR}/backends
  ${P4C_SOURCE_DIR}/extensions
  ${P4C_SOURCE_DIR}
  ${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest/include
  ${P4C_BINARY_DIR}
  )
add_definitions (-DCONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}")
add_definitions (-DCONFIG_PKGDATADIR="${P4C_ARTIFACTS_OUTPUT_DIRECTORY}")

set (CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${P4C_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${P4C_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${P4C_CXX_FLAGS}")
set (CPPLINT_FILES)          # list to collect all files that need lint
set (TEST_TAGS "p4" CACHE INTERNAL "test tags") # list to collect all test tags
set (IR_DEF_FILES)           # list to collect all .def files

# Other configuration files that need to be generated
configure_file ("${P4C_SOURCE_DIR}/cmake/config.h.cmake" "${P4C_BINARY_DIR}/config.h")

set (IR_GENERATED_SRCS
  ${CMAKE_BINARY_DIR}/ir/ir-generated.h
  ${CMAKE_BINARY_DIR}/ir/ir-generated.cpp
  ${CMAKE_BINARY_DIR}/ir/gen-tree-macro.h)
set (IR_GENERATOR ${P4C_BINARY_DIR}/tools/ir-generator/irgenerator)

# the order of adding subdirectories matters because of target dependencies
add_subdirectory (tools/driver)
add_subdirectory (lib)
add_cpplint_files(lib "${LIB_CPPLINT_FILES}")
add_subdirectory (tools/ir-generator)
add_cpplint_files(tools/ir-generator "${IRGEN_CPPLINT_FILES}")
add_subdirectory (ir)
add_cpplint_files(ir "${IR_CPPLINT_FILES}")

# component libraries: must be defined before being used in the
# backend executables
set (P4C_LIBRARIES frontend midend controlplane ir p4ctoolkit)

# add extensions - before the frontends as they produce IR and extra frontend
set(EXTENSION_FRONTEND_SOURCES)
file (GLOB p4c_extensions RELATIVE ${P4C_SOURCE_DIR}/extensions ${P4C_SOURCE_DIR}/extensions/*)
MESSAGE ("-- Available extensions ${p4c_extensions}")
foreach (ext ${p4c_extensions})
  if (EXISTS ${P4C_SOURCE_DIR}/extensions/${ext}/CMakeLists.txt)
    add_subdirectory (extensions/${ext})
    set (__cpplint_list "${${ext}_CPPLINT_FILES}")
    list (LENGTH __cpplint_list __list_len)
    if (__list_len GREATER 0)
      add_cpplint_files(extensions/${ext} "${${ext}_CPPLINT_FILES}")
    endif()
  endif()
endforeach(ext)

add_subdirectory (frontends)
add_cpplint_files(frontends "${FRONTEND_CPPLINT_FILES}")
add_subdirectory (midend)
add_cpplint_files(midend "${MIDEND_CPPLINT_FILES}")
add_subdirectory (control-plane)
add_cpplint_files(control-plane "${CONTROLPLANE_CPPLINT_FILES}")


add_subdirectory (backends/bmv2)
add_cpplint_files(backends/bmv2 "${BMV2_CPPLINT_FILES}")
add_subdirectory (backends/ebpf)
add_cpplint_files(backends/ebpf "${EBPF_CPPLINT_FILES}")
add_subdirectory (backends/p4test)
add_cpplint_files(backends/p4test "${P4TEST_CPPLINT_FILES}")

#if (ENABLE_P4RUNTIME_TO_PD)
#  add_subdirectory (p4runtime-to-pd)
#endif()

# IR Generation
# \TODO: Add all the fixup code. Is it still needed?
set_source_files_properties(${IR_GENERATOR} PROPERTIES GENERATED TRUE)
add_custom_command (OUTPUT ${IR_GENERATED_SRCS}
  COMMAND ${IR_GENERATOR} -i ir/ir-generated.cpp -o ir/ir-generated.h -t ir/gen-tree-macro.h ${IR_DEF_FILES}
  MAIN_DEPENDENCY ${IR_GENERATOR}
  COMMENT "Generating IR class files")

add_custom_target(genIR DEPENDS ${IR_GENERATED_SRCS})

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

# tags, etags
set (CTAGS_DIRS backends extensions frontends ir lib tools midend)
add_custom_target(tags
  COMMAND ctags -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp $(CTAGS_DIRS)
  COMMAND cd tools/ir-generator && ctags -R --langmap=Flex:+.l,YACC:+.ypp . ../../lib
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "Generating ctags")
add_custom_target(etags
  COMMAND ctags -e -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp $(CTAGS_DIRS)
  COMMAND cd tools/ir-generator && ctags -e -R --langmap=Flex:+.l,YACC:+.ypp . ../../lib
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "Generating extended ctags")

# check, recheck, check-*, check-ifail, gtest
set (P4C_TEST_FLAGS --test-timeout 90 --output-on-failure -j 4)
add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS}
  COMMAND ${CMAKE_MAKE_PROGRAM} cpplint-quiet
  COMMENT "Running tests")

add_custom_target(recheck
  COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --rerun-failed
  COMMENT "Re-running failed tests")

list (REMOVE_DUPLICATES TEST_TAGS)
foreach(t ${TEST_TAGS})
  add_custom_target(check-${t}
    COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --tests-regex "${t}/.*"
    COMMENT "Running tests for tag ${t}")
  add_custom_target(recheck-${t}
    COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --tests-regex "${t}/.*" --rerun-failed
    COMMENT "Re-running failed tests for tag ${t}")
endforeach()

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
  install (FILES ${P4C_BINARY_DIR}/doxygen_out/html
    RESOURCE DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/docs)
endif()

# install
# package

# Force the execution of all hacks that are needed because of
# compatibility with automake. Needs to go into bootstrap.
# execute_process(${CMAKE_MAKE_COMMAND} p4c_driver)

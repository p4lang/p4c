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
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PREFIX})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PREFIX})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PREFIX})
set (CMAKE_USE_RELATIVE_PATHS 1)

OPTION (WITH_CCACHE "Enable ccache to save compilation time" OFF)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  OPTION (WITH_LIBCXX "Enable libc++ for Clang" ON)
else ()
  OPTION (WITH_LIBCXX "Enable libc++ for Clang" OFF)
endif ()
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
add_definitions (-DCONFIG_PREFIX="${CMAKE_PREFIX}")
add_definitions (-DCONFIG_PKGDATADIR="${CMAKE_DATADIR}")

set (CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${P4C_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${P4C_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${P4C_CXX_FLAGS}")
set (CPPLINT_FILES)          # list to collect all files that need lint
set (TEST_TAGS)              # list to collect all test tags

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
add_subdirectory (frontends)
add_cpplint_files(frontends "${FRONTEND_CPPLINT_FILES}")
add_subdirectory (midend)
add_cpplint_files(midend "${MIDEND_CPPLINT_FILES}")
add_subdirectory (control-plane)
add_cpplint_files(control-plane "${CONTROLPLANE_CPPLINT_FILES}")

# component libraries: must be after the targets for libraries are defined
# and before being used in the backend executables
set (P4C_LIBRARIES frontend midend controlplane ir p4ctoolkit)

add_subdirectory (backends/bmv2)
add_cpplint_files(backends/bmv2 "${BMV2_CPPLINT_FILES}")
add_subdirectory (backends/ebpf)
add_cpplint_files(backends/ebpf "${EBPF_CPPLINT_FILES}")
add_subdirectory (backends/p4test)
add_cpplint_files(backends/p4test "${P4TEST_CPPLINT_FILES}")
#if (ENABLE_P4RUNTIME_TO_PD)
#  add_subdirectory (p4runtime-to-pd)
#endif()

# Need to add all the fixup code
set_source_files_properties(${IR_GENERATOR} PROPERTIES GENERATED TRUE)
add_custom_command (OUTPUT ${IR_GENERATED_SRCS}
  COMMAND ${IR_GENERATOR} -i ir/ir-generated.cpp -o ir/ir-generated.h -t ir/gen-tree-macro.h ${IR_DEF_FILES}
  MAIN_DEPENDENCY ${IR_GENERATOR}
  COMMENT "Generating IR class files")

add_custom_target(genIR DEPENDS ${IR_GENERATED_SRCS})

# cpplint
list ( SORT CPPLINT_FILES )
set (CPPLINT_CMD ${P4C_SOURCE_DIR}/tools/cpplint.py)
set (CPPLINT_ARGS --root=${P4C_SOURCE_DIR} --extensions=h,hpp,cpp,ypp,l)
add_custom_target(cpplint
  COMMAND ${CPPLINT_CMD} ${CPPLINT_ARGS} ${CPPLINT_FILES}
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "cpplint"
  )
add_custom_target(cpplint-quiet
  COMMAND ${CPPLINT_CMD} --quiet ${CPPLINT_ARGS} ${CPPLINT_FILES}
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "cpplint quietly"
  )

# tags, etags
set (CTAGS_DIRS backends extensions frontends ir lib tools midend)
add_custom_target(tags
  COMMAND ctags -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp $(CTAGS_DIRS)
  COMMAND cd tools/ir-generator && ctags -R --langmap=Flex:+.l,YACC:+.ypp . ../../lib
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "Generating ctags"
  )
add_custom_target(etags
  COMMAND ctags -e -R --langmap=C++:+.def,Flex:+.l,YACC:+.ypp $(CTAGS_DIRS)
  COMMAND cd tools/ir-generator && ctags -e -R --langmap=Flex:+.l,YACC:+.ypp . ../../lib
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "Generating extended ctags"
  )

# check, recheck, check-*, check-ifail, gtest
set (P4C_TEST_FLAGS --test-timeout 90 --output-on-failure -j 4)
add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS}
  COMMAND ${CMAKE_MAKE_PROGRAM} cpplint-quiet
  COMMENT "Running tests")

add_custom_target(recheck
  COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --rerun-failed
  COMMENT "Re-running failed tests")

foreach(t ${TEST_TAGS})
  add_custom_target(check-${t}
    COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --tests-regex "${t}/.*"
    COMMENT "Running tests for tag ${t}")
  add_custom_target(recheck-${t}
    COMMAND ${CMAKE_CTEST_COMMAND} ${P4C_TEST_FLAGS} --tests-regex "${t}/.*" --rerun-failed
    COMMENT "Re-running failed tests for tag ${t}")
endforeach()

# docs
if (DOXYGEN_FOUND)
  if(DOXYGEN_DOT_FOUND)
    set (HAVE_DOT 'YES')
  else()
    set (HAVE_DOT 'NO')
  endif()
  set (DOXYGEN_FILE ${P4C_SOURCE_DIR}/docs/doxygen/doxygen.cfg)
  add_custom_target(docs
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
endif(DOXYGEN_FOUND)

# install
# package

# Force the execution of all hacks that are needed because of
# compatibility with automake. Needs to go into bootstrap.
# execute_process(${CMAKE_MAKE_COMMAND} p4c_driver)

# Copyright (C) 2024 Intel Corporation
# 
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy
# of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations under the License.
# 
# 
# SPDX-License-Identifier: Apache-2.0

find_package(PkgConfig QUIET)
pkg_check_modules(LIBRAPIDJSON REQUIRED RapidJSON=1.1.0)

# spdlog can be included header only
# it cannot be build normally as it uses newer cmake
set (LIBSPDLOG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/spdlog/include")

set (LOGGING_SCHEMA_FILES
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/event_log_schema
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/phv_schema
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/power_schema
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/resources_schema
  ${BFN_P4C_SOURCE_DIR}/compiler_interfaces/schemas/source_info_schema
)

set (LOGGING_SRCS)
foreach(f IN LISTS LOGGING_SCHEMA_FILES)
  list (APPEND LOGGING_SRCS ${f}.py)
  get_filename_component(__fname ${f} NAME)
  list (APPEND GEN_LOGGING_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${__fname}.h)
endforeach()

set_source_files_properties(
  ${GEN_LOGGING_HDRS} PROPERTIES GENERATED TRUE)

set (GEN_LOGGING ${CMAKE_CURRENT_SOURCE_DIR}/generate_logging.py)
add_custom_command(OUTPUT ${GEN_LOGGING_HDRS}
  COMMAND ${GEN_LOGGING} -o ${CMAKE_CURRENT_BINARY_DIR} ${LOGGING_SRCS}
  DEPENDS ${LOGGING_SRCS} ${GEN_LOGGING}
  COMMENT "Generating logging code for ${LOGGING_SRCS}")

add_custom_target(genLogging
  DEPENDS ${GEN_LOGGING_HDRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR} ${LIBRAPIDJSON_INCLUDE_DIRS})
add_executable(test_phv_logging test_phv.cpp ${BFN_P4C_SOURCE_DIR}/bf-p4c/common/run_id.cpp)
add_dependencies(test_phv_logging genLogging)
target_link_libraries(test_phv_logging PRIVATE absl::strings)

add_test(NAME test_phv_logging COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_phv_logging)

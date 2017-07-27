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

set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix ${CMAKE_INSTALL_PREFIX})
set (datarootdir "\${prefix}/share")
configure_file ("p4runtime-to-pd.in" "${CMAKE_CURRENT_BINARY_DIR}/p4runtime-to-pd" @ONLY)

# p4runtime-to-pd is a script to generate PD files directly from P4Runtime. The
# output is compatible with p4c-bm's output.
set (P4RUNTIME_TO_PD_SRCS
  __init__.py
  main.py
  tenjin_wrapper.py
  ${P4C_BINARY_DIR}/control-plane/p4/config/p4info_pb2.py
  ${P4C_BINARY_DIR}/control-plane/p4/config/v1model_pb2.py
  )

install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/p4runtime-to-pd
  DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (FILES ${P4RUNTIME_TO_PD_SRCS}
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4runtime_to_pd_src)
install (DIRECTORY templates
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4runtime_to_pd_src/p4runtime-to-pd
  FILES_MATCHING PATTERN "*.h" PATTERN "*.cpp" PATTERN "*.ipp" PATTERN "*.thrift")

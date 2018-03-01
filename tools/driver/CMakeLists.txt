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

# bin_SCRIPTS += p4c
# CLEANFILES += $(bin_SCRIPTS)
set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix ${CMAKE_INSTALL_PREFIX})
set (datarootdir "\${prefix}/share")
configure_file ("p4c.in" "${P4C_BINARY_DIR}/p4c" @ONLY)
execute_process(COMMAND chmod a+x ${P4C_BINARY_DIR}/p4c)


set (P4C_DRIVER_SRCS
  p4c_src/main.py
  p4c_src/driver.py
  p4c_src/util.py
  p4c_src/config.py
  p4c_src/__init__.py
  )

set (P4C_TARGET_CFGS)
if (ENABLE_BMV2)
  list (APPEND P4C_TARGET_CFGS p4c_src/p4c.bmv2.cfg)
endif()
if (ENABLE_EBPF)
  list (APPEND P4C_TARGET_CFGS p4c_src/p4c.ebpf.cfg)
endif()

install (PROGRAMS ${P4C_BINARY_DIR}/p4c
  DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY p4c_src
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}
  FILES_MATCHING PATTERN "*.py")
install (FILES ${P4C_TARGET_CFGS}
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4c_src)

set (P4C_DRIVER_DST)
foreach (__f IN LISTS P4C_DRIVER_SRCS P4C_TARGET_CFGS)
  list (APPEND P4C_DRIVER_DST "${P4C_BINARY_DIR}/${__f}")
endforeach(__f)

add_custom_command(OUTPUT ${P4C_DRIVER_DST}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4c_src &&
          for f in ${P4C_DRIVER_SRCS} ${P4C_TARGET_CFGS} \; do
          ${CMAKE_COMMAND} -E copy_if_different \$$f ${P4C_BINARY_DIR}/p4c_src \;
          done
  COMMAND chmod a+x ${P4C_BINARY_DIR}/p4c
  DEPENDS ${P4C_DRIVER_SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Copying p4c driver"
  )

add_custom_target(p4c_driver ALL DEPENDS ${P4C_DRIVER_DST})

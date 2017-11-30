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
  p4c_src/p4c.bmv2.cfg
  p4c_src/p4c.ebpf.cfg
  )

install (PROGRAMS ${P4C_BINARY_DIR}/p4c
  DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY p4c_src
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}
  FILES_MATCHING PATTERN "*.py" PATTERN "*.cfg")

# p4cdir=$(pkgdatadir)/p4c_src

set (P4C_DRIVER_DST)
foreach (__f IN LISTS P4C_DRIVER_SRCS)
  list (APPEND P4C_DRIVER_DST "${P4C_BINARY_DIR}/${__f}")
endforeach(__f)

add_custom_command(OUTPUT ${P4C_DRIVER_DST}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4c_src &&
          for f in ${P4C_DRIVER_SRCS} \; do
          ${CMAKE_COMMAND} -E copy_if_different \$$f ${P4C_BINARY_DIR}/p4c_src \;
          done
  COMMAND chmod a+x ${P4C_BINARY_DIR}/p4c
  # hack to make the tests work. If the p4include directory is
  # provided with -I to the compiler the output files include the
  # contents of the includes. If the compiler finds them in the
  # current directory, it leaves the #include directive in the output,
  # and this is how the current sample outputs are stored.
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${P4C_SOURCE_DIR}/p4include ${P4C_BINARY_DIR}/p4include
  # hack to link the generated executables at the top level directory
  # because the test run scripts expect them there. This should go
  # away when removing automake and runscripts can be adjusted.
  # COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/backends/bmv2/p4c-bm2-ss ${P4C_BINARY_DIR}/p4c-bm2-ss
  # COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/backends/p4test/p4test ${P4C_BINARY_DIR}/p4test
  # COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/backends/ebpf/p4c-ebpf ${P4C_BINARY_DIR}/p4c-ebpf
  DEPENDS ${P4C_DRIVER_SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Copying p4c driver"
  )

add_custom_target(p4c_driver ALL DEPENDS ${P4C_DRIVER_DST})

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# P4C generates bindings with its selected Protobuf, so it needs the installed
# schemas, not precompiled P4Runtime libraries. Prefer the upstream CMake
# package, with a fallback for older schema-only installations.
find_package(p4runtime CONFIG QUIET)

# Accept a prefix through
# p4runtime_ROOT/CMAKE_PREFIX_PATH, or an explicit p4runtime_PROTO_DIR.
if(NOT p4runtime_FOUND)
  find_path(p4runtime_PROTO_DIR
    NAMES p4/v1/p4runtime.proto
    PATH_SUFFIXES share/p4runtime/proto share/p4runtime include proto
    DOC "Directory containing the installed P4Runtime p4/ schema tree")
  mark_as_advanced(p4runtime_PROTO_DIR)
endif()

set(p4runtime_SCHEMAS_FOUND TRUE)
foreach(schema p4/config/v1/p4info.proto p4/config/v1/p4types.proto
               p4/v1/p4runtime.proto p4/v1/p4data.proto google/rpc/status.proto)
  if(NOT EXISTS "${p4runtime_PROTO_DIR}/${schema}")
    set(p4runtime_SCHEMAS_FOUND FALSE)
  endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(p4runtime
  REQUIRED_VARS p4runtime_PROTO_DIR p4runtime_SCHEMAS_FOUND
  REASON_FAILURE_MESSAGE
    "Set p4runtime_ROOT to the installation prefix or p4runtime_PROTO_DIR to the complete schema tree containing p4/.")

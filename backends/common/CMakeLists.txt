# SPDX-FileCopyrightText: 2024 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

set(BACKENDS_COMMON_SRCS
    metermap.cpp
    programStructure.cpp
    portableProgramStructure.cpp
    psaProgramStructure.cpp
)

add_library(backends-common STATIC ${BACKENDS_COMMON_SRCS})

target_link_libraries(backends-common
  # These libraries are exposed by a header.
  PUBLIC absl::bits
  PUBLIC absl::strings
  PUBLIC ${LIBGC_LIBRARIES}
  PUBLIC ${P4C_LIBRARIES}
)

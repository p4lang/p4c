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

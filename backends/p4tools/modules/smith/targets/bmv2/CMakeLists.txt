if(ENABLE_TESTING)
  # Include the test subdirectory.
  message("-- Adding p4smith bmv2 test suite")
  include(test/P4Tests.cmake)
endif()

# Source files for smith.
set(
  SMITH_SOURCES
  ${SMITH_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/psa.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/v1model.cpp
  PARENT_SCOPE
)

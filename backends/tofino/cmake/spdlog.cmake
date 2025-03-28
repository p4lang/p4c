message(STATUS "Fetching spdlog")

# Preserve previous FETCHCONTENT_QUIET setting
set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
set(FETCHCONTENT_QUIET OFF)

set(SPDLOG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog)

# Check if the source directory exists.
if(EXISTS ${SPDLOG_SOURCE_DIR}/CMakeLists.txt)
    # If it exists but wasn't built before, manually add it.
    set(FETCHCONTENT_SOURCE_DIR_SPDLOG ${SPDLOG_SOURCE_DIR})
    # Avoid fetching again.
    set(FETCHCONTENT_UPDATES_DISCONNECTED_SPDLOG ON)
endif()

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.8.3
  SOURCE_DIR ${SPDLOG_SOURCE_DIR}
  USES_TERMINAL_DOWNLOAD TRUE
  GIT_PROGRESS TRUE
)

FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)
  add_subdirectory(${SPDLOG_SOURCE_DIR} ${CMAKE_BINARY_DIR}/spdlog)
endif()

# Restore FETCHCONTENT_QUIET setting
set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})

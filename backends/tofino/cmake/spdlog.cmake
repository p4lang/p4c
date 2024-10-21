message(STATUS "Fetching spdlog")

include(FetchContent)

# Preserve previous FETCHCONTENT_QUIET setting
set(FETCHCONTENT_QUIET_PREV ${FETCHCONTENT_QUIET})
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.8.3
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog
  USES_TERMINAL_DOWNLOAD TRUE
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(spdlog)

# Restore FETCHCONTENT_QUIET setting
set(FETCHCONTENT_QUIET ${FETCHCONTENT_QUIET_PREV})

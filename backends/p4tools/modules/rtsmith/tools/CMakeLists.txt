# ##################################################################################################
# The Flay Checker - Add only when Flay is installed.
# ##################################################################################################
if (TARGET flay)
  set(FLAY_CHECKER flay_checker.cpp)
  set (FLAY_CHECKER_DRIVER ${CMAKE_CURRENT_BINARY_DIR}/rtsmith_flay_checker)
  add_executable(rtsmith_flay_checker ${FLAY_CHECKER})
  target_link_libraries(
    rtsmith_flay_checker PRIVATE flay rtsmith ${CMAKE_THREAD_LIBS_INIT}
  )
endif()

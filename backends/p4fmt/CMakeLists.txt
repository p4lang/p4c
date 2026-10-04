set(FMT_SRCS
    p4fmt.cpp
    options.cpp
    main.cpp
    formatTokens.cpp
    syntaxRecovery.cpp
    sourceParser.cpp
    preprocessor.cpp
)

set(REFCHECK_SRCS
    refcheck.cpp
    options.cpp
    p4fmt.cpp
    formatTokens.cpp
    syntaxRecovery.cpp
    sourceParser.cpp
    preprocessor.cpp
)

# p4fmt
add_executable(p4fmt ${FMT_SRCS})
target_link_libraries(p4fmt ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4fmt frontend)

install (TARGETS p4fmt
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(p4formatter
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4fmt ${P4C_BINARY_DIR}/p4fmt
)

# p4fmt - reference checker
add_executable(checkfmt ${REFCHECK_SRCS})
target_link_libraries(checkfmt ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

add_custom_target(p4refchecker
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/checkfmt ${P4C_BINARY_DIR}/checkfmt
)

add_dependencies(p4c_driver p4formatter p4refchecker)

add_custom_target(p4fmt-check
    COMMAND ${PYTHON_EXECUTABLE} ${P4C_SOURCE_DIR}/tools/format-p4.py
        --formatter $<TARGET_FILE:p4fmt> --check --fail-on-fallback ${P4C_SOURCE_DIR}/testdata
    DEPENDS p4fmt
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Checking P4 formatting; rejecting fallback and printing formatting diffs"
    VERBATIM
)
# Explicit network-backed acceptance target; ordinary CTest runs remain offline.
add_custom_target(p4fmt-external-check
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/external_corpus.py
        --formatter $<TARGET_FILE:p4fmt> --cache ${P4C_BINARY_DIR}/p4fmt-external
    DEPENDS p4fmt
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Checking pinned DASH/PINS formatting, preservation, and active coverage"
    VERBATIM
)
add_custom_target(p4fmt-fix-errors
    COMMAND ${PYTHON_EXECUTABLE} ${P4C_SOURCE_DIR}/tools/format-p4.py
        --formatter $<TARGET_FILE:p4fmt> --write --fail-on-fallback ${P4C_SOURCE_DIR}/testdata
    DEPENDS p4fmt
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Formatting P4 files in testdata"
    VERBATIM
)

add_test(NAME p4fmt-format-check
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/format_check.py
        $<TARGET_FILE:p4fmt> ${P4C_SOURCE_DIR}/tools/format-p4.py
)

add_test(NAME p4fmt-source-preservation
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/source_preservation.py
        $<TARGET_FILE:p4fmt> $<TARGET_FILE:checkfmt> ${P4C_SOURCE_DIR}/testdata
)

add_test(NAME p4fmt-corpus
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/corpus.py
        $<TARGET_FILE:p4fmt> ${P4C_SOURCE_DIR}/testdata
        --report ${CMAKE_CURRENT_BINARY_DIR}/corpus-report.json
)
set_tests_properties(p4fmt-corpus PROPERTIES LABELS "p4fmt;corpus" PROCESSORS 4 TIMEOUT 900)

if (ENABLE_GTESTS)
    add_executable(p4fmt-source-tests tests/source_test.cpp formatTokens.cpp syntaxRecovery.cpp preprocessor.cpp)
    target_link_libraries(p4fmt-source-tests PRIVATE gtest_main)
    add_test(NAME p4fmt-source-tokens COMMAND p4fmt-source-tests)
endif ()

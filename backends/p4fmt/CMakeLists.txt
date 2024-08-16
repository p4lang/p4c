set(FMT_SRCS
    p4fmt.cpp
    options.cpp
    main.cpp
    p4formatter.cpp
)

set(REFCHECK_SRCS
    refcheck.cpp
    options.cpp
    p4fmt.cpp
    p4formatter.cpp
)

# p4fmt
add_executable(p4fmt ${FMT_SRCS})
target_link_libraries(p4fmt ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4fmt frontend)

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

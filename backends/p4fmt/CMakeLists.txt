set(FMT_SRCS
    p4fmt.cpp
    options.cpp
)

add_executable(p4fmt ${FMT_SRCS})
target_link_libraries(p4fmt ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4fmt frontend)

add_custom_target(p4formatter
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4fmt ${P4C_BINARY_DIR}/p4fmt
)

add_dependencies(p4c_driver p4formatter)

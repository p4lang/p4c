cmake_minimum_required(VERSION 3.10)

# Retrieves the kernel version and stores it in KERNEL_VER.
function(get_kernel_version OUT_VERSION)
    execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_VER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE rc
    )

    if (rc)
        message(FATAL_ERROR "Failed to detect kernel version using uname.")
    endif()

    message(STATUS "Detected kernel version: ${KERNEL_VER}")

    # Extract full version in MAJOR.MINOR.PATCH format
    string(REGEX MATCH "^([0-9]+\\.[0-9]+\\.[0-9]+)" MATCHED_VERSION ${KERNEL_VER})

    if (MATCHED_VERSION)
        set(${OUT_VERSION} ${MATCHED_VERSION} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Failed to parse kernel version from uname output.")
    endif()
endfunction()

# Checks whether the kernel version is at least MIN_VERSION.
function(check_minimum_kernel_version MIN_VERSION RESULT_VAR)
    get_kernel_version(CURRENT_VERSION)

    message(STATUS "Comparing kernel version ${CURRENT_VERSION} with required ${MIN_VERSION}")

    if (CURRENT_VERSION VERSION_LESS MIN_VERSION)
        message(STATUS "Kernel version ${CURRENT_VERSION} is less than ${MIN_VERSION}")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
    else()
        message(STATUS "Kernel version ${CURRENT_VERSION} meets or exceeds ${MIN_VERSION}")
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
    endif()
endfunction()

cmake_minimum_required(VERSION 3.10)

# Retrieves the kernel version from /usr/include/linux/version.h and stores it in KERNEL_VER.
# This should also correspond with the linux-libc version.
# TODO: Consider other paths for linux/version.h?
function(get_linux_libc_version_from_headers OUT_VERSION)
    # macOS does not support these kinds of checks.
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        return()
    endif()

    set(LINUX_VERSION_HEADER "/usr/include/linux/version.h")

    if (NOT EXISTS ${LINUX_VERSION_HEADER})
        message(ERROR "linux/version.h not found! Ensure linux-libc-dev is installed.")
    endif()

    # Read the LINUX_VERSION_CODE line
    file(STRINGS ${LINUX_VERSION_HEADER} VERSION_LINE REGEX "#define LINUX_VERSION_CODE .*")

    if (VERSION_LINE MATCHES "#define LINUX_VERSION_CODE ([0-9]+)")
        set(LINUX_VERSION_CODE ${CMAKE_MATCH_1})
    else()
        message(ERROR "Failed to extract LINUX_VERSION_CODE from ${LINUX_VERSION_HEADER}")
    endif()

    # Convert LINUX_VERSION_CODE to major.minor.patch.
    math(EXPR KERNEL_MAJOR "${LINUX_VERSION_CODE} / 65536")
    math(EXPR KERNEL_MINOR "(${LINUX_VERSION_CODE} / 256) % 256")
    math(EXPR KERNEL_PATCH "${LINUX_VERSION_CODE} % 256")

    set(KERNEL_VERSION "${KERNEL_MAJOR}.${KERNEL_MINOR}.${KERNEL_PATCH}")

    message(STATUS "Detected kernel version from headers: ${KERNEL_VERSION}")

    set(${OUT_VERSION} ${KERNEL_VERSION} PARENT_SCOPE)
endfunction()

# Retrieves the kernel version from  uname -r and stores it in KERNEL_VER.
function(get_kernel_version_from_uname OUT_VERSION)
    # macOS does not support these kinds of checks.
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        return()
    endif()

    execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_VER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE rc
    )

    if (rc)
        message(ERROR "Failed to detect kernel version using uname.")
    endif()

    # Extract full version in MAJOR.MINOR.PATCH format
    string(REGEX MATCH "^([0-9]+\\.[0-9]+\\.[0-9]+)" MATCHED_VERSION ${KERNEL_VER})

    if (MATCHED_VERSION)
        message(STATUS "Detected kernel version from uname: ${MATCHED_VERSION}")
        set(${OUT_VERSION} ${MATCHED_VERSION} PARENT_SCOPE)
    else()
        message(ERROR "Failed to parse kernel version from uname output.")
    endif()
endfunction()

# Checks whether the linux libc version is at least MIN_VERSION.
function(check_minimum_linux_libc_version MIN_VERSION RESULT_VAR)
    # macOS does not support these kinds of checks.
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(STATUS "Skipping kernel version check on macOS.")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        return()
    endif()

    # Try getting the linux libc version from headers.
    get_linux_libc_version_from_headers(CURRENT_VERSION)

    message(STATUS "Comparing libc version ${CURRENT_VERSION} with required ${MIN_VERSION}")

    if (CURRENT_VERSION VERSION_LESS MIN_VERSION)
        message(STATUS "libc version ${CURRENT_VERSION} is less than ${MIN_VERSION}")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
    else()
        message(STATUS "libc version ${CURRENT_VERSION} meets or exceeds ${MIN_VERSION}")
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
    endif()
endfunction()

# Checks whether the kernel version is at least MIN_VERSION.
function(check_minimum_kernel_version MIN_VERSION RESULT_VAR)
    # macOS does not support these kinds of checks.
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(STATUS "Skipping kernel version check on macOS.")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        return()
    endif()

    # Try getting the version from uname.
    get_kernel_version_from_uname(CURRENT_VERSION)

    message(STATUS "Comparing kernel version ${CURRENT_VERSION} with required ${MIN_VERSION}")

    if (CURRENT_VERSION VERSION_LESS MIN_VERSION)
        message(STATUS "Kernel version ${CURRENT_VERSION} is less than ${MIN_VERSION}")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
    else()
        message(STATUS "Kernel version ${CURRENT_VERSION} meets or exceeds ${MIN_VERSION}")
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
    endif()
endfunction()

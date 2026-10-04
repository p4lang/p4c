# SPDX-FileCopyrightText: 2023 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Scan each tree once, then select C++ extensions.
set(P4C_LINT_PATTERNS
  backends/*
  control-plane/*
  frontends/*
  ir/*
  lib/*
  midend/*
  test/*
  tools/*
)
# Match the platform's glob case sensitivity.
if(WIN32 OR APPLE)
  set(P4C_LINT_EXTENSIONS "\\.([cC][pP][pP]|[hH])$")
else()
  set(P4C_LINT_EXTENSIONS "\\.(cpp|h)$")
endif()
# Filter some folders from the CI checks.
set(P4C_LINT_EXCLUDES
  "backends/ebpf/runtime"
  "backends/ubpf/runtime"
  "control-plane/p4runtime"
  "backends/tc/runtime"
  "test/frameworks"
  "backends/tofino/third_party"
)

# Discover files only when a lint target runs. C++ linters share one source scan.
set(P4C_LINT_SCRIPT "${P4C_BINARY_DIR}/CMakeFiles/p4c-lint-files.cmake")
add_custom_target(p4c-lint-cpp-files
  COMMAND ${CMAKE_COMMAND} -DLINT_KIND=CPP -P "${P4C_LINT_SCRIPT}"
  BYPRODUCTS "${P4C_BINARY_DIR}/cpplint_files.txt" "${P4C_BINARY_DIR}/clang_format_files.txt"
  VERBATIM
  COMMENT "Collecting C++ files for linting.")

#################### CPPLINT
# Retrieve the global cpplint property, including files registered by extensions.
get_property(CPPLINT_FILES GLOBAL PROPERTY CPPLINT-files)

# File lists avoid the shell argument limit. Lint targets generate them on demand.
set(CPPLINT_TXT_FILE "${P4C_BINARY_DIR}/cpplint_files.txt")
set(CPPLINT_CMD ${P4C_SOURCE_DIR}/tools/cpplint.py)
set(CPPLINT_ARGS --root=${P4C_SOURCE_DIR} --extensions=h,hpp,cpp,ypp,l)
# Why "tr '\;' '\\0'"?
# File lists use CMake's semicolon separator. Convert it to NULL bytes to preserve spaces
# and quotes in filenames. Reading the list through 'tr' make sure this also works on macOS.
add_custom_target(
  cpplint
  COMMAND tr '\;' '\\0' < ${CPPLINT_TXT_FILE} | xargs -0 -r ${CPPLINT_CMD} ${CPPLINT_ARGS}
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "cpplint"
)
add_custom_target(
  cpplint-quiet
  COMMAND tr '\;' '\\0' < ${CPPLINT_TXT_FILE} | xargs -0 -r ${CPPLINT_CMD} ${CPPLINT_ARGS} --quiet
  WORKING_DIRECTORY ${P4C_SOURCE_DIR}
  COMMENT "cpplint quietly"
)

add_dependencies(cpplint p4c-lint-cpp-files)
add_dependencies(cpplint-quiet p4c-lint-cpp-files)

#################### CLANG-FORMAT
find_program(CLANG_FORMAT_CMD clang-format)
if(CLANG_FORMAT_CMD)
  # Retrieve the global clang-format property.
  get_property(CLANG_FORMAT_FILES GLOBAL PROPERTY CLANG_FORMAT-files)
  set(CLANG_FORMAT_TXT_FILE "${P4C_BINARY_DIR}/clang_format_files.txt")
  add_custom_target(
    clang-format
    COMMAND tr '\;' '\\0' < ${CLANG_FORMAT_TXT_FILE} | xargs -0 -r ${CLANG_FORMAT_CMD} --verbose --Werror --dry-run -i -- || (echo ${RED}clang-format failed. Run \"make clang-format-fix-errors\" to fix the complaints.${COLOURRESET} && false)
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Checking files for correct clang-format formatting."
  )
  add_custom_target(
    clang-format-fix-errors
    COMMAND tr '\;' '\\0' < ${CLANG_FORMAT_TXT_FILE} | xargs -0 -r ${CLANG_FORMAT_CMD} --verbose -i --
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Formatting files using clang-format."
  )

  add_dependencies(clang-format p4c-lint-cpp-files)
  add_dependencies(clang-format-fix-errors p4c-lint-cpp-files)
else()
  message(WARNING "clang-format executable not found. Disabling clang-format checks. clang-format can be installed with \"pip3 install --user --upgrade clang-format\" or your distribution's package manager.")
endif()


#################### CLANG-TIDY

# TODO: Add files here once clang-tidy is fast enough.
# file(
#   GLOB_RECURSE P4C_CLANG_TIDY_LINT_LIST  FOLLOW_SYMLINKS
# )
# add_clang_tidy_files(${P4C_SOURCE_DIR} "${P4C_CLANG_TIDY_LINT_LIST}")

find_program(CLANG_TIDY_CMD clang-tidy)

if(CLANG_TIDY_CMD)
  # Retrieve the global clang-tidy property.
  get_property(CLANG_TIDY_FILES GLOBAL PROPERTY CLANG_TIDY-files)

  if (DEFINED CLANG_TIDY_FILES AND NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    message(WARNING "CMAKE_EXPORT_COMPILE_COMMANDS is not set ON, clang-tidy remains disabled.")
  endif()
  if(DEFINED CLANG_TIDY_FILES AND CMAKE_EXPORT_COMPILE_COMMANDS)

    # Write the list to a file.
    # We need to do this as too many files will reach the shell argument limit.
    set(CLANG_TIDY_TXT_FILE ${P4C_BINARY_DIR}/clang_tidy_files.txt)
    list(SORT CLANG_TIDY_FILES)
    file(WRITE ${CLANG_TIDY_TXT_FILE} "${CLANG_TIDY_FILES}")
    add_custom_target(
      clang-tidy
      COMMAND tr '\;' '\\0' < ${CLANG_TIDY_TXT_FILE} | xargs -0 -r ${CLANG_TIDY_CMD} -p ${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Applying clang-tidy analysis."
    )
    add_custom_target(
      clang-tidy-apply-fix-its
      COMMAND tr '\;' '\\0' < ${CLANG_TIDY_TXT_FILE} | xargs -0 -r ${CLANG_TIDY_CMD} -p ${CMAKE_BINARY_DIR} --fix
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Applying clang-tidy fix-its."
    )
  endif()
else()
  message(WARNING "clang-tidy executable not found. Disabling clang-tidy checks. clang-tidy can be installed with \"pip3 install --user --upgrade clang-tidy\" or your distribution's package manager.")
endif()


#################### BLACK

set(P4C_PYTHON_LINT_PATTERNS
  backends/*.py
  test/cmake/*.py
  testdata/*.py
  tools/*.py
)
set(P4C_PYTHON_LINT_EXCLUDES
  "backends/p4tools/submodules"
  "backends/ebpf/runtime/contrib"
  "backends/tofino/third_party"
  "tools/cpplint.py"
  "backends/tc/runtime"
)

find_program(BLACK_CMD black)
find_program(ISORT_CMD isort)

# Black and isort share the same files and should be run together.
if(BLACK_CMD AND ISORT_CMD)
  # Retrieve the global black property.
  get_property(BLACK_FILES GLOBAL PROPERTY BLACK-files)
  set(BLACK_TXT_FILE "${P4C_BINARY_DIR}/BLACK_files.txt")
  add_custom_target(p4c-lint-python-files
    COMMAND ${CMAKE_COMMAND} -DLINT_KIND=PYTHON -P "${P4C_LINT_SCRIPT}"
    BYPRODUCTS "${BLACK_TXT_FILE}"
    VERBATIM
    COMMENT "Collecting Python files for linting.")
  add_custom_target(
    black
    COMMAND tr '\;' '\\0' < ${BLACK_TXT_FILE} | xargs -0 -r ${BLACK_CMD} --config ${P4C_SOURCE_DIR}/pyproject.toml --check --diff -- || (echo ${RED}black failed. Run \"make black-fix-errors\" to fix the complaints.${COLOURRESET} && false)
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Checking files for correct black formatting."
  )
  add_custom_target(
    black-fix-errors
    COMMAND tr '\;' '\\0' < ${BLACK_TXT_FILE} | xargs -0 -r ${BLACK_CMD} --config ${P4C_SOURCE_DIR}/pyproject.toml --
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Formatting files using black."
  )
  add_custom_target(
    isort
    COMMAND tr '\;' '\\0' < ${BLACK_TXT_FILE} | xargs -0 -r ${ISORT_CMD} --check --diff -- || (echo ${RED}isort failed. Run \"make isort-fix-errors\" to fix the complaints.${COLOURRESET} && false)
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Checking files for correct isort formatting."
  )
  add_custom_target(
    isort-fix-errors
    COMMAND tr '\;' '\\0' < ${BLACK_TXT_FILE} | xargs -0 -r ${ISORT_CMD} --
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "Formatting files using isort."
  )

  add_dependencies(black p4c-lint-python-files)
  add_dependencies(black-fix-errors p4c-lint-python-files)
  add_dependencies(isort p4c-lint-python-files)
  add_dependencies(isort-fix-errors p4c-lint-python-files)
else()
  message(WARNING "black or isort executable not found. Disabling black/isort checks. black/isort can be installed with \"pip3 install --user --upgrade black\" and \"pip3 install --user --upgrade isort\" or your distribution's package manager. ")
endif()

# Pass backend registrations and scan settings to the build-time script.
configure_file("${CMAKE_CURRENT_LIST_DIR}/LintFiles.cmake.in" "${P4C_LINT_SCRIPT}" @ONLY)

#################### IWYU
if(ENABLE_IWYU)
  # Set up IWYU for P4C.
  message("Enabling IWYU checks.")
  find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
  set(iwyu_path
      ${iwyu_path}
      -Xiwyu
      --max_line_length=100
      -Xiwyu
      --no_fwd_decls
      -Xiwyu
      --cxx17ns
      -Xiwyu
      --mapping_file=${P4C_SOURCE_DIR}/tools/iwyu_mappings/p4c.imp
  )
  message("IWYU command: ${iwyu_path}")


  get_all_targets(ALL_IWYU_TARGETS ${CMAKE_CURRENT_SOURCE_DIR})
  # Apply IWYU to all targets.
  get_all_targets(ALL_IWYU_TARGETS)
  # Remove generated files from IWYU.
  list(FILTER ALL_IWYU_TARGETS EXCLUDE REGEX "controlplane-gen")
  list(FILTER ALL_IWYU_TARGETS EXCLUDE REGEX "dpdk_runtime")
  list(FILTER ALL_IWYU_TARGETS EXCLUDE REGEX "ir-generated")
  list(FILTER ALL_IWYU_TARGETS EXCLUDE REGEX "genIR")
  list(FILTER ALL_IWYU_TARGETS EXCLUDE REGEX "parser-gen")
  list(FILTER ALL_IWYU_TARGETS EXCLUDE REGEX "gtest")
  message("Applying IWYU to targets: ${all_targets}")
  foreach(target ${ALL_IWYU_TARGETS})
    set_property(TARGET ${target} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
  endforeach()

endif()

file(
  GLOB_RECURSE P4C_LINT_LIST  FOLLOW_SYMLINKS
  backends/*.cpp
  backends/*.h
  control-plane/*.cpp
  control-plane/*.h
  frontends/*.cpp
  frontends/*.h
  ir/*.cpp
  ir/*.h
  lib/*.cpp
  lib/*.h
  midend/*.cpp
  midend/*.h
  test/*.cpp
  test/*.h
  tools/*.cpp
  tools/*.h
)
# Filter some folders from the CI checks.
list(FILTER P4C_LINT_LIST EXCLUDE REGEX "backends/p4tools/submodules")
list(FILTER P4C_LINT_LIST EXCLUDE REGEX "backends/ebpf/runtime")
list(FILTER P4C_LINT_LIST EXCLUDE REGEX "backends/ubpf/runtime")
list(FILTER P4C_LINT_LIST EXCLUDE REGEX "control-plane/p4runtime")
list(FILTER P4C_LINT_LIST EXCLUDE REGEX "backends/tc/runtime")
list(FILTER P4C_LINT_LIST EXCLUDE REGEX "test/frameworks")

#################### CPPLINT
add_cpplint_files(${P4C_SOURCE_DIR} "${P4C_LINT_LIST}")

# Retrieve the global cpplint property.
get_property(CPPLINT_FILES GLOBAL PROPERTY cpplint-files)

if(DEFINED CPPLINT_FILES)
  # Write the list to a file. We need to do this as too many files will reach
  # the shell argument limit.
  set(CPPLINT_TXT_FILE ${P4C_BINARY_DIR}/cpplint_files.txt)
  file(WRITE ${CPPLINT_TXT_FILE} "${CPPLINT_FILES}")
  list(SORT CPPLINT_FILES)
  set(CPPLINT_CMD ${P4C_SOURCE_DIR}/tools/cpplint.py)
  set(CPPLINT_ARGS --root=${P4C_SOURCE_DIR} --extensions=h,hpp,cpp,ypp,l)
  # Pipe the file into cpplint.
  add_custom_target(
    cpplint
    COMMAND xargs -a ${CPPLINT_TXT_FILE} -r -d '\;' ${CPPLINT_CMD} ${CPPLINT_ARGS}
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "cpplint"
  )
  add_custom_target(
    cpplint-quiet
    COMMAND xargs -a ${CPPLINT_TXT_FILE} -r -d '\;' ${CPPLINT_CMD} ${CPPLINT_ARGS} --quiet
    WORKING_DIRECTORY ${P4C_SOURCE_DIR}
    COMMENT "cpplint quietly"
  )
endif()

#################### CLANG-FORMAT
add_clang_format_files(${P4C_SOURCE_DIR} "${P4C_LINT_LIST}")

find_program(CLANG_FORMAT_CMD clang-format)

if(NOT ${CLANG_FORMAT_CMD})
  # Retrieve the global clang-format property.
  get_property(CLANG_FORMAT_FILES GLOBAL PROPERTY clang-format-files)
  if(DEFINED CLANG_FORMAT_FILES)
    # Write the list to a file.
    # We need to do this as too many files will reach the shell argument limit.
    set(CLANG_FORMAT_TXT_FILE ${P4C_BINARY_DIR}/clang_format_files.txt)
    list(SORT CLANG_FORMAT_FILES)
    file(WRITE ${CLANG_FORMAT_TXT_FILE} "${CLANG_FORMAT_FILES}")
    set(CLANG_FORMAT_CMD clang-format)
    add_custom_target(
      clang-format
      COMMAND xargs -a ${CLANG_FORMAT_TXT_FILE} -r -d '\;' ${CLANG_FORMAT_CMD} --verbose --Werror --dry-run -i --
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Checking files for correct clang-format formatting."
    )
    add_custom_target(
      clang-format-fix-errors
      COMMAND xargs -a ${CLANG_FORMAT_TXT_FILE} -r -d '\;' ${CLANG_FORMAT_CMD} --verbose -i --
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Formatting files using clang-format."
    )
  endif()
else()
  message(WARNING "clang-format executable not found. Disabling clang-format checks. clang-format can be installed with \"pip3 install --user --upgrade clang-format\"")
endif()


#################### BLACK

file(
  GLOB_RECURSE P4C_PYTHON_LINT_LIST  FOLLOW_SYMLINKS
  backends/*.py
  tools/*.py
)
list(FILTER P4C_PYTHON_LINT_LIST EXCLUDE REGEX "backends/p4tools/submodules")
list(FILTER P4C_PYTHON_LINT_LIST EXCLUDE REGEX "backends/ebpf/runtime/contrib/libbpf")
list(FILTER P4C_PYTHON_LINT_LIST EXCLUDE REGEX "tools/cpplint.py")

add_black_files(${P4C_SOURCE_DIR} "${P4C_PYTHON_LINT_LIST}")

find_program(BLACK_CMD black)
find_program(ISORT_CMD isort)

# Black and isort share the same files and should be run together.
if(NOT ${BLACK_CMD} OR NOT (NOT ${ISORT_CMD}))
  # Retrieve the global black property.
  get_property(BLACK_FILES GLOBAL PROPERTY black-files)
  if(DEFINED BLACK_FILES)
    # Write the list to a file.
    # We need to do this as too many files will reach the shell argument limit.
    set(BLACK_TXT_FILE ${P4C_BINARY_DIR}/BLACK_files.txt)
    list(SORT BLACK_FILES)
    file(WRITE ${BLACK_TXT_FILE} "${BLACK_FILES}")
    set(BLACK_CMD black)
    add_custom_target(
      black
      COMMAND xargs -a ${BLACK_TXT_FILE} -r -d '\;' ${BLACK_CMD} --check --diff --
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Checking files for correct black formatting."
    )
    add_custom_target(
      black-fix-errors
      COMMAND xargs -a ${BLACK_TXT_FILE} -r -d '\;' ${BLACK_CMD} --
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Formatting files using black."
    )
    add_custom_target(
      isort
      COMMAND xargs -a ${BLACK_TXT_FILE} -r -d '\;' ${ISORT_CMD} --check --diff --
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Checking files for correct isort formatting."
    )
    add_custom_target(
      isort-fix-errors
      COMMAND xargs -a ${BLACK_TXT_FILE} -r -d '\;' ${ISORT_CMD} --
      WORKING_DIRECTORY ${P4C_SOURCE_DIR}
      COMMENT "Formatting files using isort."
    )
  endif()
else()
  message(WARNING "black or isort executable not found. Disabling black/isort checks. black/isort can be installed with \"pip3 install --user --upgrade black\" and \"pip3 install --user --upgrade isort\" ")
endif()

# uninstall
find_file (
  INSTALL_MANIFEST_LOCATION "install_manifest.txt"
  PATHS "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_BINARY_DIR}/.."
)

if(NOT INSTALL_MANIFEST_LOCATION)
  message(FATAL_ERROR "Cannot find install_manifest.txt")
endif(NOT INSTALL_MANIFEST_LOCATION)

file(READ "${INSTALL_MANIFEST_LOCATION}" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
  message(STATUS "Uninstalling $ENV{DESTDIR}${file}")
  if(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
    exec_program(
      "${CMAKE_COMMAND}" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
      OUTPUT_VARIABLE rm_out
      RETURN_VALUE rm_retval
      )
    if(NOT "${rm_retval}" STREQUAL 0)
      message(FATAL_ERROR "Problem when removing $ENV{DESTDIR}${file}")
    endif(NOT "${rm_retval}" STREQUAL 0)
  else(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
    message(STATUS "File $ENV{DESTDIR}${file} does not exist.")
  endif(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
endforeach(file)


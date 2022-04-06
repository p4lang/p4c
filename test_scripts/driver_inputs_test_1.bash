#!/bin/bash

### a simple test: ensure that a single non-existant input pathname results in an error output that mentions that pathname.



if [ "$0" = bash -o "$0" = "-bash" -o "$0" = dash -o "$0" = sh ] || echo $0 | grep --quiet '/\(a\|ba\|da\)\?sh$'; then
  ### the regex on the preceding line matches anything ending in {‘/’ immediately followed by either “ash”, “bash”, or “dash”}
  echo 'Please do _not_ source this file, because it has an "exit" command in it.'
  return 255
fi



BAD_PATHNAME=/path/to/a/nonexistant/supposedly-P4/source/file

./p4c $BAD_PATHNAME 2>&1 | grep --ignore-case --quiet "error.*$BAD_PATHNAME"
exit_status_from_grep=$?

if [ $exit_status_from_grep -eq 0 ]; then
  echo "Test ''$0'' succeeded."
else
  echo "Test ''$0'' failed [grep returned $exit_status_from_grep]."
  exit 1
fi

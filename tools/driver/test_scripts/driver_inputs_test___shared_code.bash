### _intentionally_ no shebang line: this file is _not_ meant to ever be executable.



### The following might seem a little bit ironic here, since _this_ file […“shared_code.bash”] _is_ intended to be sourced...
###   but the scripts that source it are _not_.
function check_for_inadvisable_sourcing {
  if [ "$0" = bash -o "$0" = "-bash" -o "$0" = dash -o "$0" = sh ] || echo $0 | grep --quiet '/\(a\|ba\|da\)\?sh$'; then
    ### the regex on the preceding line matches anything ending in {‘/’ immediately followed by either “ash”, “bash”, or “dash”}
    echo 'Please do _not_ source this file, because it has an "exit" command in it.'
    return 1
  fi
  return 0
}



### This one is for reading out the tests names in a way that shows the real Bash files` basenames withOUT replacing e.g. “./” with something long.
###
### If you call it withOUT a parameter, I don`t know exactly what will happen, but I don`t think it will be _good_. ;-)

function resolve_symlink_only_of_basename {
  echo "`dirname "$1"`"/"$(basename `realpath "$1"`)"
}



function check_for_pathname_error {
  ### arg. #1: pathname
  ### arg. #2: subtest index
  ### arg. #3: number of subtests [should be a constant at the time of editing this test file, but what the hell]

  ./p4c $1 2>&1 | grep --ignore-case --quiet "error.*$1"
  exit_status_from_grep=$?

  echo -n "Subtest #$2 (out of $3) in the test script ''`resolve_symlink_only_of_basename "$0"`'' " ### DRY
  if [ $exit_status_from_grep -eq 0 ]; then
    echo 'succeeded.'
    return 0
  else
    echo "failed [grep returned $exit_status_from_grep]."
    return 1
  fi
}



function report___num_failures___and_clamp_it_to_an_inclusive_maximum_of_255 {
  if [ $num_failures -ne 0 ]; then
    echo -n "$num_failures failure"
    if [ $num_failures -eq 1 ]; then
      echo -n ' was'
    else
      echo -n 's were'
    fi
    echo " detected in the test script ''`resolve_symlink_only_of_basename "$0"`''."
  fi

  ### Clamping the exit code at a max. of 255, for future code that starts with a copy-paste from this file and may experience >255 errors
  ### [reminder: a positive multiple of 256 is a problem, since those all map to zero and zero means “everything was OK”].
  if [ $num_failures -gt 255 ]; then num_failures=255; fi
}



return 0 ### belt and suspenders ;-)

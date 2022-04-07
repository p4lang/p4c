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

function check_for_pathname_error {
  ### arg. #1: pathname
  ### arg. #2: subtest index
  ### arg. #3: number of subtests [should be a constant at the time of editing this test file, but what the hell]

  ./p4c $1 2>&1 | grep --ignore-case --quiet "error.*$1"
  exit_status_from_grep=$?

  echo -n "Subtest #$2 (out of $3) in the test script ''$0'' " ### DRY
  if [ $exit_status_from_grep -eq 0 ]; then
    echo 'succeeded.'
    return 0
  else
    echo "failed [grep returned $exit_status_from_grep]."
    return 1
  fi
}

return 0 ### belt and suspenders ;-)

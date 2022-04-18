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



function check_for_pathname_error_in_P4_compiler_driver_output {
  ### required arg. #1: pathname         _of_ the driver [can be relative]
  ### required arg. #2: pathname to give _to_ the driver [can be relative]
  ### required arg. #3: subtest index
  ### required arg. #4: number of subtests [should be a constant at the time of editing the test files that use this shared file, but what the hell]

  "$1" "$2" 2>& 1 | grep --ignore-case --quiet "error.*$2"
  exit_status_from_grep=$?

  echo -n "Subtest #$3 (out of $4) in the test script ''`resolve_symlink_only_of_basename "$0"`'' " ### DRY
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



### requires a single arg./param., which had better be a pathname [doesn`t need to be absolute]
function check_if_this_seems_to_be_our_driver {
  echo "INFO: searching for the P4 compiler driver at ''$1''..." >& 2 ### reminder: “>& 2” is equivalent to “> /dev/stderr”

  if [   -L "$1" ]; then
  ### NOTE: is “-L” a GNUism?  “-h” might be the POSIX “spelling”.
  ###   Either way, it should work OK in {Bash on non-GNU OS} as long as I use Bash`s built-in ‘[’/“test”,
  ###   not e.g. “/bin/[” or “/bin/test” or “/usr/bin/[” or “/usr/bin/test”.

    echo "INFO: detected a symlink at ''$1'' [starting at ''$PWD'', if that is a relative pathname], so checking the target of the symlink." >& 2
    if realpath --version | grep --quiet GNU; then ### happy-happy-joy-joy: GNU realpath is available and is the default “realpath”
      ### try the “logical” interpretation first, i.e. give “../” components priority over symlink components
      if next_turtle=`realpath --canonicalize-existing  --logical "$1"`; then check_if_this_seems_to_be_our_driver "$next_turtle"; return; fi ### please see Knuth`s definition of recursion ;-)
      if next_turtle=`realpath --canonicalize-existing --physical "$1"`; then check_if_this_seems_to_be_our_driver "$next_turtle"; return; fi
      ### If we are “still here”, then canonicalization failed.  :-(
      echo "ERROR: failed to canonicalize the symlink ''$1'' while using GNU ''realpath''." >& 2
      return 1
    fi

    if readlink --version | grep --quiet GNU; then ### second-best: GNU readlink is available and is the default “readlink”
      if next_turtle=`readlink --canonicalize-existing "$1"`; then check_if_this_seems_to_be_our_driver "$next_turtle"; return; fi
      ### If we are “still here”, then canonicalization failed.  :-(
      echo "ERROR: failed to canonicalize the symlink ''$1'' while using GNU ''readlink''." >& 2
      return 2
    fi

    if realpath / > /dev/null; then ### I hope that the “realpath” implementations in e.g. BSD all do symlink-cycle detection, as GNU realpath does (at least as of GNU coreutils version 8.30)
      if next_turtle=`realpath "$1"`; then check_if_this_seems_to_be_our_driver "$next_turtle"; return; fi
      ### If we are “still here”, then canonicalization failed.  :-(
      echo "ERROR: failed to canonicalize the symlink ''$1'' while using [presumed non-GNU] ''realpath''." >& 2
      return 3
    fi

    ### The “readlink” in BSD [well, at least the one I _know_ of ;-)] is _extremely_ minimal, and AFAIK can`t do cycle detection,
    ###   so I`m not even going to _try_ non-GNU readlink... mainly because I don`t want to get
    ###   “INFO: searching for the P4 compiler driver”[...] until Bash runs out of function-call stack and crashes,
    ###   in the case of a symlink cycle.  At this point, I will just pretend I never detected a symlink and let it go forward.
    ###   This way, if the symlink _is_ part of a cycle, this whole process should crash in a way that is “nicer”
    ###   than flooding the output with “INFO: searching for the P4 compiler driver”[...] lines.

  fi ### Done checking for a symlink.  At this point, "$1" is either (1) _not_ a symlink or (2) a symlink we couldn`t canonicalize.

  if [ ! -e "$1" ]; then echo "INFO: not using ''$1'', because it is not found in the filesystem [starting at ''$PWD'', if that is a relative pathname]." >& 2; return 1; fi
  if [ ! -x "$1" ]; then echo "INFO: not using ''$1'', because it is not executable." >& 2; return 2; fi
  if [   -d "$1" ]; then echo "INFO: not using ''$1'', because it is a directory."    >& 2; return 3; fi
  if [ ! -s "$1" ]; then echo "INFO: not using ''$1'', because either it does not exist [starting at ''$PWD'', if that is a relative pathname] or it exists but is empty." >& 2; return 4; fi

  ### NOTE on the following: I _could_ be more strict, e.g. requiring that the output of “$foo --version” start with “p4c” or “p4c ”,
  ###   but that might be a bad idea in the long run, e.g. if the output of the driver`s version report will be changed,
  ###   e.g. to start with “P4C ” or “Version of P4C: ” instead of with “p4c ” as it is as of this writing
  ###   [and has been for some time, TTBOMK].
  if ! "$1" --version > /dev/null; then echo "INFO: not using ''$1'', because it did not accept the arg./flag/param. ''--version''" >& 2; return 5; fi

  ### OK; at this point, we have given up on finding “reasons” to reject "$1" as a supposedly-good P4 compiler driver. ;-)
  echo "INFO: accepting ''$1'' as a presumed P4 compiler driver." >& 2
  return 0
} ### end of function “check_if_this_seems_to_be_our_driver”



### IMPORTANT: do _not_ include any human-oriented “fluff” in this function`s standard-out output
function try_to_find_the_driver {
  ### NOTES re "$GITHUB_WORKSPACE", "$RUNNER_TEMP", and "$RUNNER_WORKSPACE":
  ###   these were all found by Abe in the GitHub CI/CD environment on April 14 2022
  ###
  ###   here are the values they had at that time [which explains why I am not checking "$RUNNER_TEMP" by itself]:
  ###
  ###     GITHUB_WORKSPACE=/__w/p4c/p4c
  ###     RUNNER_TEMP=/__w/_temp
  ###     RUNNER_WORKSPACE=/__w/p4c

  ### IMPORTANT: the ordering in the following *_IS_* important, and may need to be changed at a later time due to e.g. changes in the GitHub CI/CD
  to_probe=(./p4c ../p4c ../../p4c ../p4c/p4c ../../p4c/p4c)
  if [ -n "$GITHUB_WORKSPACE" ]; then to_probe+=("$GITHUB_WORKSPACE" "$GITHUB_WORKSPACE"/p4c "$GITHUB_WORKSPACE"/p4c/p4c); fi
  if [ -n "$RUNNER_WORKSPACE" ]; then to_probe+=("$RUNNER_WORKSPACE" "$RUNNER_WORKSPACE"/p4c "$RUNNER_WORKSPACE"/p4c/p4c); fi
  if [ -n "$RUNNER_TEMP"      ]; then to_probe+=(                    "$RUNNER_TEMP"''''//p4c "$RUNNER_TEMP"''''//p4c/p4c); fi

  for probe in ${to_probe[@]}; do
    if check_if_this_seems_to_be_our_driver "$probe"; then
      echo "$probe" ### IMPORTANT: do _not_ include any human-oriented “fluff” in this output
      return 0
    fi
  done

  echo "FATAL ERROR: could not find the P4 compiler driver.  Searched in the following locations..." >& 2
  for probe in ${to_probe[@]}; do
    echo "///>>>$probe<<<///" >& 2
    ### Using “///>>>$foo<<<///” to make it clear that the extra characters are just delimiters
    ###   [which, BTW, are here so that space characters, especially trailing ones, will become visible].
  done
  return 1
}



return 0 ### belt and suspenders ;-)

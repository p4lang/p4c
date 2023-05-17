#!/bin/bash

# This checks that the refpoint for each git submodule is on the respective
# branch that we are tracking.

### Begin configuration #######################################################
THIS_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

declare -A branchOverrides=(
)

### End configuration #########################################################

set -e

skipFetch=false
if [[ $# -gt 0 && "$1" == '--skip-fetch' ]] ; then
  skipFetch=true
  shift
fi

if [[ $# == 0 ]] ; then
  # No arguments given. Create a temporary file and call ourselves on each git
  # submodule. If the check fails for any submodule, the temporary file will be
  # removed.
  tmpfile="$(mktemp --tmpdir check-git-submodules.XXXXXXXXXX)"

  ${skipFetch} || git fetch --quiet origin
  git submodule --quiet foreach \
    "${THIS_DIR}"'/check-git-submodules.sh ${sm_path} ${sha1} '"${tmpfile}"
  rm "${tmpfile}" &>/dev/null
else
  sm_path="$1"
  sha1="$2"
  tmpfile="$3"
  echo "Checking submodule ${sm_path} with sha ${sha1} in folder ${PWD}."
  ${skipFetch} || git fetch --quiet origin
  # Figure out what branch we are tracking (e.g., "origin/main") and derive a
  # simple name for that branch (e.g., "main").
  trackingBranch="${branchOverrides["${sm_path}"]}"
  if [[ -z "${trackingBranch}" ]] ; then
    trackingBranch="$(git symbolic-ref refs/remotes/origin/HEAD | sed s%^refs/remotes/%%)"
    simpleBranchName="${trackingBranch:7}"
  else
    simpleBranchName="${trackingBranch}"
  fi

  # Check that the top of the branch being tracked is an ancestor of the
  # current refpoint.
  if ! git merge-base --is-ancestor "${sha1}" "${trackingBranch}" ; then
    echo "Submodule ${sm_path} is not on ${simpleBranchName} because ${sha1} is not an ancestor of ${trackingBranch}."

    # Remove the temporary file to signal an error. We don't use the exit
    # status for this because it would cause `git submodule foreach` to stop
    # looping, and we'd like to continue to check all remaining submodules.
    rm -f "${tmpfile}"
  fi
fi

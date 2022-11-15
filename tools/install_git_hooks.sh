#!/bin/bash

# Installs git hooks for p4c.

umask 0022
set -e

baseDir="$(realpath "$(dirname "$0")")/.."

pushd "${baseDir}"
if [[ -d ${baseDir}/.git/hooks ]] ; then
  for hook in ${baseDir}/tools/hooks/* ; do
    # Skip the README file.
    if [[ "${hook: -3}" == ".md" ]] ; then
        continue
    fi
    echo "Installing hook ${hook}"
    # Corner case for when scripts/hooks is empty.
    [[ -e "${hook}" ]] || continue

    hook_base="$(basename "${hook}")"
    [[ ! -e "${baseDir}/.git/hooks/${hook}" ]] && ln -sf "${hook}" "${baseDir}/.git/hooks/${hook_base}"

    # Ensure we finish with a success even when the above '-e' test fails.
    true
  done
fi

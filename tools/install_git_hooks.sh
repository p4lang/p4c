#!/bin/bash

# Installs git hooks for p4c.

umask 0022
set -e
baseDir="$(realpath "$(dirname "$0")")"
gitDir="$(git rev-parse --show-superproject-working-tree)/.git"

pushd "${baseDir}" > /dev/null
if [[ -d ${gitDir}/hooks ]] ; then
  for hook in ${baseDir}/hooks/* ; do
    # Skip the README file.
    if [[ "${hook: -3}" == ".md" ]] ; then
        continue
    fi
    # Corner case for when scripts/hooks is empty.
    [[ -e "${hook}" ]] || continue

    hook_dir="${gitDir}/hooks/$(basename "${hook}")"
    if [[ ! -e "${hook_dir}" ]] ; then
      echo "Installing hook \"${hook}\" to \"${hook_dir}\""
      ln -sf "${hook}" "${hook_dir}"
    fi
    # Ensure we finish with a success even when the above '-e' test fails.
    true
  done
fi

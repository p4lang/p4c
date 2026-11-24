#!/bin/bash

# SPDX-FileCopyrightText: 2022 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Installs git hooks for p4c.

umask 0022
set -e
baseDir="$(realpath "$(dirname "$0")")"

# Check if p4c is part of a superproject, otherwise use the repository git dir.
superproject="$(git rev-parse --show-superproject-working-tree 2>/dev/null || true)"
if [[ -n "${superproject}" && -d "${superproject}/.git/hooks" ]] ; then
  gitDir="${superproject}/.git"
else
  gitDir="$(git rev-parse --git-dir)"
fi
gitDir="$(realpath "${gitDir}")"

pushd "${baseDir}" > /dev/null
if [[ -d "${gitDir}/hooks" ]] ; then
  for hook in ${baseDir}/hooks/* ; do
    # Skip the README file.
    if [[ "${hook: -3}" == ".md" ]] ; then
        continue
    fi
    # Corner case for when scripts/hooks is empty.
    [[ -e "${hook}" ]] || continue

    hook_dir="${gitDir}/hooks/$(basename "${hook}")"
    if [[ ! -e "${hook_dir}" && ! -L "${hook_dir}" ]] ; then
      echo "Installing hook \"${hook}\" to \"${hook_dir}\""
      ln -sf "${hook}" "${hook_dir}"
    else
      # Update symlink to ensure it points to the current hook script.
      ln -sf "${hook}" "${hook_dir}"
      echo "Hook \"${hook_dir}\" already installed (symlink updated)"
    fi
    # Ensure we finish with a success even when the above '-e' test fails.
    true
  done
else
  echo "Error: Git hooks directory not found at \"${gitDir}/hooks\"" >&2
  exit 1
fi

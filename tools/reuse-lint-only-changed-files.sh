#! /bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

THIS_SCRIPT_FILE_MAYBE_RELATIVE="$0"
THIS_SCRIPT_DIR_MAYBE_RELATIVE="${THIS_SCRIPT_FILE_MAYBE_RELATIVE%/*}"
THIS_SCRIPT_DIR_ABSOLUTE=`readlink -f "${THIS_SCRIPT_DIR_MAYBE_RELATIVE}"`

GITHUB_BASE_REF="$1"

# Select only files that were added in the PR, not existing files that
# were modified.

# We do not want to slow people down in creating and merging good PRs
# if they modify any of the few remaining files that do not yet have a
# copyright and license annotation, and thus would fail the `reuse
# lint-file` check.

# We _do_ want them to add a copyright and license annotation to any
# new file they add to the repository.

git diff --name-status --no-renames origin/${GITHUB_BASE_REF}...HEAD | "${THIS_SCRIPT_DIR_ABSOLUTE}/print-added-files.py" | xargs uvx reuse lint-file
ec=$?
if [ $ec -ne 0 ]; then
    echo "::error::New or modified files in this PR, listed above, failed copyright or license checks.  See https://github.com/p4lang/.github/blob/main/copyright-and-license-issues/README-choosing-copyright-holder-and-license-for-new-files.md"
fi

exit $ec

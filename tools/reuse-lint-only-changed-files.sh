#! /bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

THIS_SCRIPT_FILE_MAYBE_RELATIVE="$0"
THIS_SCRIPT_DIR_MAYBE_RELATIVE="${THIS_SCRIPT_FILE_MAYBE_RELATIVE%/*}"
THIS_SCRIPT_DIR_ABSOLUTE=`readlink -f "${THIS_SCRIPT_DIR_MAYBE_RELATIVE}"`

GITHUB_BASE_REF="$1"

set -x
git diff --name-only origin/${GITHUB_BASE_REF}...HEAD | "${THIS_SCRIPT_DIR_ABSOLUTE}/print-existing-files.sh" | xargs uvx reuse lint-file
ec=$?
set +x
if [ $ec -ne 0 ]; then
    echo "::error::New or modified files in this PR, listed above, failed copyright or license checks.  See https://github.com/p4lang/.github/blob/main/copyright-and-license-issues/README-choosing-copyright-holder-and-license-for-new-files.md"
fi

echo "----------------------------------------"
echo "contents of $HOME/out-debug.txt:"
cat $HOME/out-debug.txt
echo "----------------------------------------"

exit $ec

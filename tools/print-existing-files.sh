#! /bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Among all of the command line arguments, print only those which name
# files that exist.

# Loop through each line coming from standard input
while IFS= read -r filename; do
    # Skip empty lines to prevent errors
    [[ -z "$filename" ]] && continue
    # Always quote your variables to safely handle spaces in file names
    if [[ -f "$filename" ]]; then
        echo "$filename"
    fi
done

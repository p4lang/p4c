#! /bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Among all of the command line arguments, print only those which name
# files that exist.

echo "print-existing-files.sh: creating debug log file" > $HOME/out-debug.txt

# Loop through each line coming from standard input
while IFS= read -r filename; do
    # Skip empty lines to prevent errors
    [[ -z "$filename" ]] && continue

    echo "Processing file: $filename"

    # Always quote your variables to safely handle spaces in file names
    if [[ -f "$filename" ]]; then
        echo "-> File exists:$filename:" >> $HOME/out-debug.txt
        echo "$filename"
    else
        echo "-> No such file:$filename:" >> $HOME/out-debug.txt
    fi
done

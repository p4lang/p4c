#! /usr/bin/env python3

# This program expects standard input to be the output of a command
# similar to this one:

# git diff --name-status --no-renames BASE_REF...HEAD_REF

# Each line consists of a one-letter filter character, a tab, and a
# path name of a file involved in the changes between the two git
# versions compared.  The one-letter codes are documented in the help
# provided by `git diff --help`.  In particular, we only want to
# include files that were added in the output of this program, not
# files that were modified or deleted.  Those files have a filter
# character of 'A'.

import fileinput
import re
import sys

for line in fileinput.input():
    match = re.search(r"^([A-Z])\t(.*)$", line)
    if match:
        status = match.group(1)
        filename = match.group(2)
        if status == 'A':
            print(filename)
    else:
        print(f"Unrecognized input line format: {line.rstrip()}", file=sys.stderr)

#! /bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Among all of the command line arguments, print only those which name
# files that exist.

for j in $*
do
    if [ -e "$j" ]
    then
	1>&2 echo "File exists: $j"
        echo "$j"
    else
	1>&2 echo "NO file: $j"
    fi
done

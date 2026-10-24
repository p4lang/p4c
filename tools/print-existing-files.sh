#! /bin/bash

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

# Among all of the command line arguments, print only those which name
# files that exist.

echo "Create the file ..." > $HOME/out-debug.txt
for j in $*
do
    if [ -e "$j" ]
    then
	echo "File exists: $j" >> $HOME/out-debug.txt
        echo "$j"
    else
	echo "NO file: $j" >> $HOME/out-debug.txt
    fi
done

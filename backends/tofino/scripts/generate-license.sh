#!/bin/bash

# Produces LICENSE from LICENSE.in.

set -e

licenceTemplate="LICENSE.in"
licenceOutput="LICENSE"

# Change to repository root.
cd "$(realpath "$(dirname "$0")/..")"

if [[ ! -f "${licenceTemplate}" ]] ; then
  echo "${licenceTemplate} not found in $(pwd)" >&2
  exit 1
fi

sed \
  -e '/^#.*$/d'                      `# Strip comment lines.` \
  -e 's/\s*$//'                      `# Trim trailing space on each line.` \
  -e "s/@YEAR@/$(date +%Y)/g"        `# Substitute year.` \
  -e '/./,$!d'                       `# Delete blank lines at top of file.` \
  -e :a -e '/^\n*$/{$d;N;};/\n$/ba'  `# Delete blank lines at end of file.` \
  "${licenceTemplate}" \
  | fmt -w 80  `# Re-wrap to 80 columns.` \
  >"${licenceOutput}"

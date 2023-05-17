#!/usr/bin/env bash
#
# Debian/Ubuntu Packaging Scripts
# Copyright (C) 2002-2021 by Thomas Dreibholz
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Contact: dreibh@iem.uni-due.de

# ====== Path variables =========================================
FILE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
P4C_DIR="${FILE_DIR}/../.."
DEBIAN_DIR="${P4C_DIR}/debian"

# ====== Obtain package information =========================================
CHANGELOG_HEADER="`head -n1 ${DEBIAN_DIR}/changelog`"
PACKAGE=`echo ${CHANGELOG_HEADER} | sed -e "s/(.*//" -e "s/ //g"`


# ====== Clean up ===========================================================
rm -f *.deb *.dsc *.asc *.changes *.build *.upload *.tar.gz stamp-h* svn-commit* *~

for type in gz bz2 xz ; do
   find . -maxdepth 1 -name "${PACKAGE}-*.${type}" | xargs -r rm
   find . -maxdepth 1 -name "${PACKAGE}_*.${type}" | xargs -r rm
done
find . -maxdepth 1 -name "*.buildinfo" | xargs -r rm

shopt -s extglob
rm -rf ${PACKAGE}-+([0-9]).+([0-9]).+([0-9])*

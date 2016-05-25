#!/bin/bash

# Copyright 2013-present Barefoot Networks, Inc.
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

#
# Antonin Bas (antonin@barefootnetworks.com)
#
#

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

version_str="$(cat $THIS_DIR/../VERSION | tr -d '\n')"

bf_path="$THIS_DIR/../VERSION-build"
bversion_str=""
if [ -f $bf_path ]; then
    bversion_str="$(cat $bf_path | tr -d '\n')"
else
    git_sha_str="$(git rev-parse @)"
    if [ $? -ne 0 ]; then
        bversion_str="unknown"
    else
        bversion_str=${git_sha_str:0:8}
    fi
fi

version_str="$version_str-$bversion_str"

# Omit the trailing newline, so that m4_esyscmd can use the result directly.
printf %s "$version_str"

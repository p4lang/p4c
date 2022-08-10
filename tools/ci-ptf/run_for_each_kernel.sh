#!/bin/bash -e

# Copyright 2022-present Orange
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Script that starts tests on every kernel

for version in $KERNEL_VERSIONS; do
  ./tools/ci-ptf/run_test.sh "$version" "$@"
  exit_code=$?
  if [ $exit_code -ne 0 ]; then
    echo "************************************************************"
    echo "Test failed for kernel $version"
    echo "************************************************************"
    exit $exit_code
  fi
done

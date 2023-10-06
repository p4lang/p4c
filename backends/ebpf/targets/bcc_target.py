#!/usr/bin/env python3
# Copyright 2013-present Barefoot Networks, Inc.
# Copyright 2018 VMware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
from pathlib import Path

from .target import EBPFTarget

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../../tools")))
import testutils


class Target(EBPFTarget):
    def __init__(self, tmpdir, options, template):
        EBPFTarget.__init__(self, tmpdir, options, template)

    def compile_dataplane(self):
        # Not implemented yet, just pass the test
        return testutils.SUCCESS

    def check_outputs(self):
        # Not implemented yet, just pass the test
        return testutils.SUCCESS

    def run(self):
        # Not implemented yet, just pass the test
        return testutils.SUCCESS

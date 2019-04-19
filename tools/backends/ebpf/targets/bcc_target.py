#!/usr/bin/env python2
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

import os
import sys
from target import EBPFTarget
sys.path.insert(0, os.path.dirname(
    os.path.realpath(__file__)) + '/../../../tools')
from testutils import *


class Target(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def compile_dataplane(self):
        # Not implemented yet, just pass the test
        return SUCCESS

    def check_outputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS

    def run(self):
        # Not implemented yet, just pass the test
        return SUCCESS

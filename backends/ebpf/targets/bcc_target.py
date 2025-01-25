#!/usr/bin/env python3
# Copyright 2013-present Barefoot Networks, Inc.
# Copyright 2018 VMware, Inc.
# SPDX-License-Identifier: GPL-2.0-only
# Reason-GPL: import-scapy via target

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

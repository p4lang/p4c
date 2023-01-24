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
from glob import glob
from pathlib import Path
from .target import EBPFTarget

# path to the tools folder of the compiler
# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../../tools")))
import testutils


class Target(EBPFTarget):

    def __init__(self, tmpdir, options, template):
        EBPFTarget.__init__(self, tmpdir, options, template)

    def compile_dataplane(self):
        args = self.get_make_args(self.runtimedir, self.options.target)
        # List of bpf programs to attach to the interface
        args += "BPFOBJ=" + self.template + " "
        args += "CFLAGS+=-DCONTROL_PLANE "
        # these files are specific to the test target
        args += "SOURCES+=%s/ebpf_registry.c " % self.runtimedir
        args += "SOURCES+=%s/ebpf_map.c " % self.runtimedir
        args += "SOURCES+=%s.c " % self.template
        # include the src of libbpf directly, does not require installation
        args += "INCLUDES+=-I%s/contrib/libbpf/src " % self.runtimedir
        if self.options.extern:
            # we inline the extern so we need a direct include
            args += "INCLUDES+=-include" + self.options.extern + " "
            # need to include the temporary dir because of the tmp import
            args += "INCLUDES+=-I" + self.tmpdir + " "
        errmsg = "Failed to build the filter:"
        return testutils.exec_process(args, errmsg).returncode

    def run(self):
        testutils.log.info("Running model")
        direction = "in"
        pcap_pattern = self.filename("", direction)
        num_files = len(glob(self.filename("*", direction)))
        testutils.log.info(f"Input file: {pcap_pattern}")
        # Main executable
        args = self.template + " "
        # Input pcap pattern
        args += "-f " + pcap_pattern + " "
        # Number of input interfaces
        args += "-n " + str(num_files) + " "
        # Debug flag (verbose output)
        args += "-d"
        errmsg = "Failed to execute the filter:"
        result = testutils.exec_process(args, errmsg).returncode
        return result

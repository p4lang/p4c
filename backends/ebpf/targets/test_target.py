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
        args += f"BPFOBJ={self.template} "
        args += "CFLAGS+=-DCONTROL_PLANE "
        # these files are specific to the test target
        args += f"SOURCES+={ self.runtimedir}/ebpf_registry.c "
        args += f"SOURCES+={ self.runtimedir}/ebpf_map.c "
        args += f"SOURCES+={self.template}.c "
        # include the src of libbpf directly, does not require installation
        args += f"INCLUDES+=-I{self.runtimedir}/contrib/libbpf/src "
        if self.options.extern:
            # we inline the extern so we need a direct include
            args += f"INCLUDES+=-include{self.options.extern} "
            # need to include the temporary dir because of the tmp import
            args += f"INCLUDES+=-I{self.tmpdir} "
        result = testutils.exec_process(args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to build the filter")
        return result.returncode

    def run(self):
        testutils.log.info("Running model")
        direction = "in"
        pcap_pattern = self.filename("", direction)
        num_files = len(glob(self.filename("*", direction)))
        testutils.log.info("Input file: %s", pcap_pattern)
        # Main executable
        args = f"{self.template} "
        # Input pcap pattern
        args += f"-f {pcap_pattern} "
        # Number of input interfaces
        args += f"-n {num_files} "
        # Debug flag (verbose output)
        args += "-d"
        result = testutils.exec_process(args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to execute the filter")
        return result.returncode

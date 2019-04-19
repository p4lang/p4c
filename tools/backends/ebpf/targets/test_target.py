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
from glob import glob
from target import EBPFTarget
# path to the tools folder of the compiler
sys.path.insert(0, os.path.dirname(
    os.path.realpath(__file__)) + '/../../../tools')
from testutils import *


class Target(EBPFTarget):
  def __init__(self, tmpdir, options, template, outputs):
    EBPFTarget.__init__(self, tmpdir, options, template, outputs)

  def compile_dataplane(self):
    args = self.get_make_args(self.runtimedir, self.options.target)
    # List of bpf programs to attach to the interface
    args += "BPFOBJ=" + self.template + " "
    args += "CFLAGS+=-DCONTROL_PLANE "
    errmsg = "Failed to build the filter:"
    return run_timeout(self.options.verbose, args, TIMEOUT,
                       self.outputs, errmsg)

  def run(self):
    report_output(self.outputs["stdout"],
                  self.options.verbose, "Running model")
    direction = "in"
    pcap_pattern = self.filename('', direction)
    num_files = len(glob(self.filename('*', direction)))
    report_output(self.outputs["stdout"],
                  self.options.verbose,
                  "Input file: %s" % pcap_pattern)
    # Main executable
    args = self.template + " "
    # Input pcap pattern
    args += "-f " + pcap_pattern + " "
    # Number of input interfaces
    args += "-n " + str(num_files) + " "
    # Debug flag (verbose output)
    args += "-d"
    errmsg = "Failed to execute the filter:"
    result = run_timeout(self.options.verbose, args,
                         TIMEOUT, self.outputs, errmsg)
    return result

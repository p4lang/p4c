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

""" Contains different eBPF models and specifies their individual behavior
    Currently five phases are defined:
   1. Invokes the specified compiler on a provided p4 file.
   2. Parses an stf file and generates an pcap output.
   3. Loads the generated template or compiles it to a runnable binary.
   4. Feeds the generated pcap test packets into the P4 "filter"
   5. Evaluates the output with the expected result from the .stf file
"""

from __future__ import print_function
import sys
import os
import tempfile
import shutil
import argparse
sys.path.insert(0, os.path.dirname(
    os.path.realpath(__file__)) + '/../../tools')
from testutils import *


PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir", help="the root directory of "
                    "the compiler source tree")
PARSER.add_argument("p4filename", help="the p4 file to process")
PARSER.add_argument("-b", "--nocleanup", action="store_false",
                    help="do not remove temporary results for failing tests")
PARSER.add_argument("-c", "--compiler", dest="compiler", default="p4c-ebpf",
                    help="Specify the path to the compiler binary, "
                    "default is p4c-ebpf")
PARSER.add_argument("-v", "--verbose", action="store_true",
                    help="verbose operation")
PARSER.add_argument("-f", "--replace", action="store_true",
                    help="replace reference outputs with newly generated ones")
PARSER.add_argument("-t", "--target", dest="target", default="test",
                    help="Specify the compiler backend target, "
                    "default is test")


def import_from(module, name):
    """ Try to import a module and class directly instead of the typical
        Python method. Allows for dynamic imports. """
    module = __import__(module, fromlist=[name])
    return getattr(module, name)


class EBPFFactory(object):
    """ Generator class.
     Returns a target subclass based on the provided target option."""
    @staticmethod
    def create(tmpdir, options, template, outputs):
        target_name = "targets." + options.target + "_target"
        target_class = "Target"
        report_output(outputs["stdout"], options.verbose,
                      "Loading target ", target_name)
        try:
            EBPFTarget = import_from(target_name, target_class)
        except ImportError as e:
            report_err(outputs["stderr"], e)
            return None
        return EBPFTarget(tmpdir, options, template, outputs)


class Options(object):
    def __init__(self):
        self.binary = ""                # This program's name
        self.cleanupTmp = True          # Remove tmp folder?
        self.compiler = ""              # Path to the P4 compiler binary
        self.p4Filename = ""            # File that is being compiled
        self.verbose = False            # Enable verbose output
        self.replace = False            # Replace previous outputs
        self.target = "test"            # The name of the target compiler
        # Actual location of the test framework
        self.testdir = os.path.dirname(os.path.realpath(__file__))


def check_path(path):
    """Checks if a path is an actual directory and converts the input
        to an absolute path"""
    if not os.path.exists(path):
        msg = "{0} does not exist".format(path)
        raise argparse.ArgumentTypeError(msg)
    else:
        return os.path.abspath(os.path.expanduser(path))


def run_model(ebpf, stffile):
    result = ebpf.generate_model_inputs(stffile)
    if result != SUCCESS:
        return result

    result = ebpf.compile_dataplane()
    if result != SUCCESS:
        return result

    result = ebpf.run()
    if result == SKIPPED:
        return SUCCESS
    if result != SUCCESS:
        return result

    result = ebpf.check_outputs()
    return result


def run_test(options, argv):
    """ Define the test environment and compile the p4 target
        Optional: Run the generated model """
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=os.path.abspath("./"))
    os.chmod(tmpdir, 0o744)
    basename = os.path.basename(options.p4filename)  # Name of the p4 test
    base, ext = os.path.splitext(basename)           # Name without the type
    dirname = os.path.dirname(options.p4filename)    # Directory of the file

    # We can do this if an *.stf file is present
    stffile = dirname + "/" + base + ".stf"
    if options.verbose:
        print("Checking for ", stffile)
    if not os.path.isfile(stffile):
        # If no stf file is present just use the empty file
        stffile = dirname + "/empty.stf"

    template = tmpdir + "/" + "test"
    output = {}
    output["stderr"] = tmpdir + "/" + basename + "-stderr"
    output["stdout"] = tmpdir + "/" + basename + "-stdout"
    report_output(output["stdout"], options.verbose,
                  "Writing temporary files into ", tmpdir)

    ebpf = EBPFFactory.create(tmpdir, options, template, output)
    if ebpf is None:
        return FAILURE
    # Compile the p4 file to the specified target
    result, expected_error = ebpf.compile_p4(argv)

    # Compile and run the generated output
    if result == SUCCESS and not expected_error:
        # If no empty.stf present, don't try to run the model at all
        if not os.path.isfile(stffile):
            msg = "No stf file present!"
            report_output(output["stdout"],
                          options.verbose, msg)
            result = SUCCESS
        else:
            result = run_model(ebpf, stffile)
    # Only if we did not expect it to fail
    if result != SUCCESS:
        return result

    # Remove the tmp folder
    if options.cleanupTmp:
        report_output(output["stdout"],
                      options.verbose, "Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result


if __name__ == '__main__':
    """ main """
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()
    options = Options()
    options.compiler = check_path(args.compiler)
    options.p4filename = check_path(args.p4filename)
    options.verbose = args.verbose
    options.replace = args.replace
    options.cleanupTmp = args.nocleanup
    options.target = args.target

    # All args after '--' are intended for the p4 compiler
    argv = argv[1:]
    # Run the test with the extracted options and modified argv
    result = run_test(options, argv)
    sys.exit(result)

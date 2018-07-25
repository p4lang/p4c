#!/usr/bin/env python
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


# Runs the p4c-ebpf compiler on a P4-16 program
# and then runs the program on specified input packets

from __future__ import print_function
import sys
import os
import stat
import tempfile
import shutil
from ebpftargets import EBPFFactory
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../../tools')
from testutils import *


class Options(object):
    def __init__(self):
        self.binary = ""                # This program's name
        self.cleanupTmp = True          # Remove tmp folder?
        self.p4Filename = ""            # File that is being compiled
        self.compilerSrcDir = ""        # Path to compiler source tree
        self.verbose = False            # Enable verbose output
        self.replace = False            # Replace previous outputs
        self.target = "bcc"             # The name of the target compiler


def isdir(path):
    try:
        return stat.S_ISDIR(os.stat(path).st_mode)
    except OSError:
        return False


def report_error(*message):
    print("***", *message)


def run_model(ebpf, stffile):

    if not os.path.isfile(stffile):
        # If no empty.stf present, don't try to run the model at all
        return SUCCESS

    result = ebpf.generate_model_inputs(stffile)
    if result != SUCCESS:
        return result

    result = ebpf.create_filter()
    if result != SUCCESS:
        return result

    result = ebpf.run()
    if result != SUCCESS:
        return result

    result = ebpf.check_outputs()
    return result


def run_test(options, argv):
    """ Define the test environment and compile the p4 target
        Optional: Run the generated model """
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=os.path.abspath("./"))
    basename = os.path.basename(options.p4filename)  # Name of the p4 test
    base, ext = os.path.splitext(basename)           # Name without the type
    dirname = os.path.dirname(options.p4filename)    # Directory of the file
    expected_dirname = dirname + "_outputs"          # Expected outputs folder

    # We can do this if an *.stf file is present
    stffile = dirname + "/" + base + ".stf"
    if options.verbose:
        print("Check for ", stffile)
    if not os.path.isfile(stffile):
        # If no stf file is present just use the empty file
        stffile = dirname + "/empty.stf"

    if options.verbose:
        print("Writing temporary files into ", tmpdir)

    template = tmpdir + "/" + "test"
    output = {}
    output["stderr"] = tmpdir + "/" + basename + "-stderr"
    output["stdout"] = tmpdir + "/" + basename + "-stdout"

    ebpf = EBPFFactory.create(tmpdir, options, template, output)

    # Compile the p4 file to the specified target
    result, expected_error = ebpf.compile_p4(argv)

    # Compile and run the generated output
    if result == SUCCESS and not expected_error:
        result = run_model(ebpf, stffile)
    # Only if we did not expect it to fail
    if result != SUCCESS:
        return result

    # Remove the tmp folder
    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result


def usage(options):
    name = options.binary
    print(name, "usage:")
    print(name, "[-t] rootdir [options] file.p4")
    print("Invokes compiler on the supplied file, possibly adding extra arguments")
    print("`rootdir` is the root directory of the compiler source tree")
    print("-t: Specify the compiler backend target, default is bcc")
    print("options:")
    print("          -b: do not remove temporary results for failing tests")
    print("          -v: verbose operation")
    print("          -f: replace reference outputs with newly generated ones")


def parse_options(argv):
    """ Parses the input arguments and stores them in the options object
        which is passed to target objects.
        TODO: This function should use the default python parse package """
    options = Options()
    options.binary = argv[0]
    if len(argv) <= 2:
        usage(options)
        sys.exit(FAILURE)

    if argv[1] == '-t':
        if len(argv) == 0:
            report_error("Missing argument for -t option")
            usage(options)
            sys.exit(FAILURE)
        else:
            options.target = argv[2]
            argv = argv[1:]
    argv = argv[1:]
    options.compilerSrcDir = argv[1]
    argv = argv[2:]
    if not os.path.isdir(options.compilerSrcDir):
        print(options.compilerSrcDir + " is not a folder", file=sys.stderr)
        usage(options)
        sys.exit(FAILURE)

    while argv[0][0] == '-':
        if argv[0] == "-b":
            options.cleanupTmp = False
        elif argv[0] == "-v":
            options.verbose = True
        elif argv[0] == "-f":
            options.replace = True
        else:
            print("Unknown option ", argv[0], file=sys.stderr)
            usage(options)
        argv = argv[1:]
    options.p4filename = argv[-1]
    argv = argv[1:]
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcDir):
        options.testName = options.p4filename[len(options.compilerSrcDir):]
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]
    return options, argv


def main(argv):
    """ main """
    # Parse options and process argv
    options, argv = parse_options(argv)
    # Run the test with the extracted options and modified argv
    result = run_test(options, argv)
    sys.exit(result)


if __name__ == "__main__":
    main(sys.argv)

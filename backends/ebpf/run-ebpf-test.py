#!/usr/bin/env python
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
# TODO: do something with the output of the compiler

from __future__ import print_function
import sys
import os
import stat
import tempfile
import shutil
import difflib

from ebpftargets import EBPFFactory


SUCCESS = 0
FAILURE = 1


class Options(object):
    def __init__(self):
        self.binary = ""                # this program's name
        self.cleanupTmp = True          # if false do not remote tmp folder created
        self.p4Filename = ""            # file that is being compiled
        self.compilerSrcDir = ""        # path to compiler source tree
        self.verbose = False
        self.replace = False            # replace previous outputs
        self.compilerOptions = []
        self.target = "bcc"             # the name of the target compiler


def usage(options):
    name = options.binary
    print(name, "usage:")
    print(name, "rootdir [options] file.p4")
    print("Invokes compiler on the supplied file, possibly adding extra arguments")
    print("`rootdir` is the root directory of the compiler source tree")
    print("options:")
    print("          -t: Specify the compiler backend target, default is bcc")
    print("          -b: do not remove temporary results for failing tests")
    print("          -v: verbose operation")
    print("          -f: replace reference outputs with newly generated ones")


def isdir(path):
    try:
        return stat.S_ISDIR(os.stat(path).st_mode)
    except OSError:
        return False


def reportError(*message):
    print("***", *message)


# Currently not in use.
def compare_files(options, produced, expected):
    if options.replace:
        if options.verbose:
            print("Saving new version of ", expected)
        shutil.copy2(produced, expected)
        return SUCCESS

    if options.verbose:
        print("Comparing", produced, "and", expected)
    diff = difflib.Differ().compare(
        open(produced).readlines(), open(expected).readlines())
    result = SUCCESS

    message = ""
    for l in diff:
        if l[0] == ' ':
            continue
        result = FAILURE
        message += l

    if message is not "":
        print("Files ", produced, " and ",
              expected, " differ:", file=sys.stderr)
        print(message, file=sys.stderr)

    return result


# Currently not in use.
def check_generated_files(options, tmpdir, expecteddir):
    files = os.listdir(tmpdir)
    for file in files:
        if options.verbose:
            print("Checking", file)
        produced = tmpdir + "/" + file
        expected = expecteddir + "/" + file
        if not os.path.isfile(expected):
            if options.verbose:
                print("Expected file does not exist; creating", expected)
            shutil.copy2(produced, expected)
        else:
            result = compare_files(options, produced, expected)
            if result != SUCCESS:
                return result
    return SUCCESS


def run_model(ebpf, stffile):

    if not os.path.isfile(stffile):
        # If no empty.stf present, don't try to run the model at all
        return SUCCESS

    result = ebpf.generate_model_inputs(stffile)
    if result != SUCCESS:
        return result

    result = ebpf.create_switch()
    if result != SUCCESS:
        return result

    result = ebpf.run()
    if result != SUCCESS:
        return result

    result = ebpf.checkOutputs()
    return result


# Define the test environment and compile the p4 target
# Optional: Run the generated model
def run_test(options, argv):
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=".")
    basename = os.path.basename(options.p4filename)  # name of the p4 test
    base, ext = os.path.splitext(basename)           # name without the type
    dirname = os.path.dirname(options.p4filename)    # directory of the file
    expected_dirname = dirname + "_outputs"          # expected outputs folder

    # We can do this if an *.stf file is present
    stffile = dirname + "/" + base + ".stf"
    if options.verbose:
        print("Check for ", stffile)
    if not os.path.isfile(stffile):
        # If no stf file is present just use the empty file
        stffile = dirname + "/empty.stf"

    if options.verbose:
        print("Writing temporary files into ", tmpdir)
    cfile = tmpdir + "/" + "test" + ".c"          # name of the target c file
    stderr = tmpdir + "/" + basename + "-stderr"  # location of error output

    ebpf = EBPFFactory.create(tmpdir, options, cfile, stderr)

    # Compile the p4 file to the specifies target
    result, expected_error = ebpf.compile_p4(argv)

    # Compile and run the generated output
    # only if we did not expect it to fail
    if result == SUCCESS and not expected_error:
        result = run_model(ebpf, stffile)

    # Remove the tmp folder
    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result


# Parse the input of the CMake script.
# TODO: This function should use the default python parse package
def parse_options(argv):
    options = Options()
    options.binary = argv[0]
    if len(argv) <= 2:
        usage(options)
        sys.exit(FAILURE)

    if argv[1] == '-t':
        if len(argv) == 0:
            reportError("Missing argument for -t option")
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
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcDir):
        options.testName = options.p4filename[len(options.compilerSrcDir):]
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]
    return options, argv


# main
def main(argv):
    # parse options and process argv
    options, argv = parse_options(argv)
    # run the test with the extracted options and modified argv
    result = run_test(options, argv)
    sys.exit(result)


if __name__ == "__main__":
    main(sys.argv)

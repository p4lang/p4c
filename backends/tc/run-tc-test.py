#!/usr/bin/env python3
# ********************************************************************************
# Copyright (C) 2023 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions
# and limitations under the License.
# *******************************************************************************/

import difflib
import os
import shutil
import stat
import sys
import tempfile
from subprocess import Popen, call
from threading import Thread

SUCCESS = 0
FAILURE = 1


class Options(object):
    def __init__(self):
        self.binary = ""  # this program's name
        self.cleanupTmp = False  # if false do not remote tmp folder created
        self.p4Filename = ""  # file that is being compiled
        self.compilerSrcDir = ""  # path to compiler source tree
        self.verbose = False
        self.replace = False  # replace previous outputs
        self.compilerOptions = []


def usage(options):
    name = options.binary
    print(name, "usage:")
    print(name, "rootdir [options] file.p4")
    print("Invokes compiler on the supplied file, possibly adding extra arguments")
    print("`rootdir` is the root directory of the compiler source tree")
    print("options:")
    print("          -b: do not remove temporary results for failing tests")
    print("          -v: verbose operation")
    print("          -f: replace reference outputs with newly generated ones")


def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename


class Local(object):
    # object to hold local vars accessible to nested functions
    pass


def run_timeout(options, args, timeout, stderr):
    if options.verbose:
        print("Executing ", " ".join(args))
    local = Local()
    local.process = None

    def target():
        procstderr = None
        if stderr is not None:
            procstderr = open(stderr, "w")
        local.process = Popen(args, stdout=procstderr, stderr=procstderr)
        local.process.wait()

    thread = Thread(target=target)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        print("Timeout ", " ".join(args), file=sys.stderr)
        local.process.terminate()
        thread.join()
    if local.process is None:
        # never even started
        if options.verbose:
            print("Process failed to start")
        return -1
    if options.verbose:
        print("Exit code ", local.process.returncode)
    return local.process.returncode


timeout = 10 * 60


def compare_files(options, produced, expected):
    if options.replace:
        if options.verbose:
            print("Saving new version of ", expected)
        shutil.copy2(produced, expected)
        return SUCCESS

    if options.verbose:
        print("Comparing ", produced, " and ", expected)
    if produced.endswith(".c"):
        sedcommand = "sed -i.bak '1d;2d' " + produced
        call(sedcommand, shell=True)
    if produced.endswith(".h"):
        sedcommand = "sed -i.bak '1d;2d' " + produced
        call(sedcommand, shell=True)
    sedcommand = "sed -i -e 's/[a-zA-Z0-9_\/\-]*testdata\///g' " + produced
    call(sedcommand, shell=True)
    diff = difflib.Differ().compare(open(produced).readlines(), open(expected).readlines())
    result = SUCCESS

    message = ""
    for l in diff:
        if l[0] == ' ':
            continue
        result = FAILURE
        message += l

    if message != "":
        print("Files ", produced, " and ", expected, " differ:", file=sys.stderr)
        print(message, file=sys.stderr)

    return result


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


def process_file(options, argv):
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=".")
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)
    expected_dirname = dirname + "_outputs"  # expected outputs are here

    if options.verbose:
        print("Writing temporary files into ", tmpdir)
    ppfile = tmpdir + "/" + base + ".template"
    cfile = tmpdir + "/"
    introfile = tmpdir + "/" + base + "_introspection.json"
    stderr = tmpdir + "/" + basename + "-stderr"

    if not os.path.isfile(options.p4filename):
        raise Exception("No such file " + options.p4filename)
    args = ["./p4c-pna-p4tc", "-o", ppfile, "-c", cfile, "-i", introfile]
    args.extend(argv)
    print("input: ", options, args, timeout, stderr)
    result = run_timeout(options, args, timeout, stderr)

    if result != SUCCESS:
        print("Error compiling")
        print(" ".join(open(stderr).readlines()))
        # If the compiler crashed fail the test
        if 'Compiler Bug' in open(stderr).readlines():
            return FAILURE

    expected_error = isError(options.p4filename)
    if expected_error:
        # invert result
        if result == SUCCESS:
            result = FAILURE
        else:
            result = SUCCESS

    if result == SUCCESS:
        result = check_generated_files(options, tmpdir, expected_dirname)

    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result


def isdir(path):
    try:
        return stat.S_ISDIR(os.stat(path).st_mode)
    except OSError:
        return False


######################### main


def main(argv):
    options = Options()
    print("P4TC testing")
    options.binary = argv[0]
    if len(argv) <= 2:
        usage(options)
        sys.exit(FAILURE)

    options.compilerSrcdir = argv[1]
    argv = argv[2:]
    if not os.path.isdir(options.compilerSrcdir):
        print(options.compilerSrcdir + " is not a folder", file=sys.stderr)
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
    if options.p4filename.startswith(options.compilerSrcdir):
        options.testName = options.p4filename[len(options.compilerSrcdir) :]
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]

    result = process_file(options, argv)
    sys.exit(result)


if __name__ == "__main__":
    main(sys.argv)

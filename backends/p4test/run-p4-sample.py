#!/usr/bin/env python
# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Runs the compiler on a sample P4 V1.2 program

from __future__ import print_function
from subprocess import Popen
from threading import Thread
import sys
import re
import os
import stat
import tempfile
import shutil
import difflib
import subprocess

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
    print("          -a \"args\": pass args to the compiler")

def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename

class Local(object):
    # object to hold local vars accessable to nested functions
    pass

def run_timeout(options, args, timeout, stderr):
    if options.verbose:
        print(" ".join(args))
    local = Local()
    local.process = None
    def target():
        procstderr = None
        if stderr is not None:
            procstderr = open(stderr, "w")
        local.process = Popen(args, stderr=procstderr)
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

timeout = 200  # seconds

def compare_files(options, produced, expected):
    if options.replace:
        if options.verbose:
            print("Saving new version of ", expected)
        shutil.copy2(produced, expected)
        return SUCCESS

    if options.verbose:
        print("Comparing", produced, "and", expected)

    # unfortunately difflib has no option to compare ignoring whitespace,
    # so we invoke diff for this purpose
    cmd = "diff -B -q -w -I \"#include\" " + produced + " " + expected
    exitcode = subprocess.call(cmd, shell=True);
    if exitcode == 0:
        return SUCCESS

    # if the files differ print the differences
    diff = difflib.Differ().compare(open(produced).readlines(), open(expected).readlines())
    result = SUCCESS

    marker = re.compile("\?[ \t\+]*");
    ignoreNextMarker = False
    message = ""
    for l in diff:
        if l.startswith("- #include") or l.startswith("+ #include"):
            # These can differ because the paths change
            ignoreNextMarker = True
            continue
        if ignoreNextMarker:
            ignoreNextMarker = False
            if marker.match(l):
                continue
        if l[0] == ' ': continue
        result = FAILURE
        message += l

    if message is not "":
        print("Files ", produced, " and ", expected, " differ:", file=sys.stderr)
        print("'", message, "'", file=sys.stderr)

    return result

def recompile_file(options, produced, mustBeIdentical):
    # Compile the generated file a second time
    secondFile = produced + "-x";
    args = ["./p4test", "-I.", "--pp", secondFile, "--p4-16", produced]
    result = run_timeout(options, args, timeout, None)
    if result != SUCCESS:
        return result
    if mustBeIdentical:
        result = compare_files(options, produced, secondFile)
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

def file_name(tmpfolder, base, suffix, ext):
    return tmpfolder + "/" + base + "-" + suffix + ext

def process_file(options, argv):
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=".")
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)
    if "_samples/" in dirname:
        expected_dirname = dirname.replace("_samples/", "_samples_outputs/", 1)
    elif "_errors/" in dirname:
        expected_dirname = dirname.replace("_errors/", "_errors_outputs/", 1)
    else:
        expected_dirname = dirname + "_outputs"  # expected outputs are here
    if not os.path.exists(expected_dirname):
        os.makedirs(expected_dirname)

    # We rely on the fact that these keys are in alphabetical order.
    rename = { "FrontEnd_11_SimplifyControlFlow": "first",
               "FrontEnd_20_SimplifyDefUse": "frontend",
               "MidEnd_27_Evaluator": "midend" }

    if options.verbose:
        print("Writing temporary files into ", tmpdir)
    ppfile = tmpdir + "/" + basename                  # after parsing
    referenceOutputs = ",".join(rename.keys())
    stderr = tmpdir + "/" + basename + "-stderr"

    if not os.path.isfile(options.p4filename):
        raise Exception("No such file " + options.p4filename)
    args = ["./p4test", "--pp", ppfile, "--dump", tmpdir, "--top4", referenceOutputs] + options.compilerOptions
    if "14_samples" in options.p4filename or "v1_samples" in options.p4filename:
        args.extend(["--p4-14"]);
    args.extend(argv)

    result = run_timeout(options, args, timeout, stderr)
    if result != SUCCESS:
        print("Error compiling")
        print("".join(open(stderr).readlines()))

    expected_error = isError(options.p4filename)
    if expected_error:
        # invert result
        if result == SUCCESS:
            result = FAILURE
        else:
            result = SUCCESS

    # Canonicalize the generated file names
    lastFile = None
    for k in sorted(rename.keys()):
        file = file_name(tmpdir, base, k, ext)
        if os.path.isfile(file):
            newName = file_name(tmpdir, base, rename[k], ext)
            os.rename(file, newName)
            lastFile = newName

    if (result == SUCCESS):
        result = check_generated_files(options, tmpdir, expected_dirname);
    if (result == SUCCESS) and (not expected_error):
        result = recompile_file(options, ppfile, False)
    if (result == SUCCESS) and (not expected_error) and (lastFile is not None):
        result = recompile_file(options, lastFile, True)

    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result

def isdir(path):
    try:
        return stat.S_ISDIR(os.stat(path).st_mode)
    except OSError:
        return False;

######################### main

def main(argv):
    options = Options()

    options.binary = argv[0]
    if len(argv) <= 2:
        usage(options)
        return FAILURE

    options.compilerSrcdir = argv[1]
    argv = argv[2:]
    if not os.path.isdir(options.compilerSrcdir):
        print(options.compilerSrcdir + " is not a folder", file=sys.stderr)
        usage(options)
        return FAILURE

    while argv[0][0] == '-':
        if argv[0] == "-b":
            options.cleanupTmp = False
        elif argv[0] == "-v":
            options.verbose = True
        elif argv[0] == "-f":
            options.replace = True
        elif argv[0] == "-a":
            if len(argv) == 0:
                print("Missing argument for -a option")
                usage(options)
                sys.exit(1)
            else:
                options.compilerOptions += argv[1].split();
                argv = argv[1:]
        else:
            print("Uknown option ", argv[0], file=sys.stderr)
            usage(options)
            sys.exit(1)
        argv = argv[1:]

    options.p4filename=argv[-1]
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcdir):
        options.testName = options.p4filename[len(options.compilerSrcdir):];
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]

    result = process_file(options, argv)
    sys.exit(result)

if __name__ == "__main__":
    main(sys.argv)

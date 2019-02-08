#!/usr/bin/env python2
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
from subprocess import Popen,PIPE
from threading import Thread
import errno
import sys
import re
import os
import stat
import tempfile
import shutil
import difflib
import subprocess
import glob

SUCCESS = 0
FAILURE = 1

class Options(object):
    def __init__(self):
        self.binary = ""                # this program's name
        self.cleanupTmp = True          # if false do not remote tmp folder created
        self.p4filename = ""            # file that is being compiled
        self.compilerSrcDir = ""        # path to compiler source tree
        self.verbose = False
        self.replace = False            # replace previous outputs
        self.dumpToJson = False
        self.compilerOptions = []
        self.runDebugger = False
        self.generateP4Runtime = False

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
    print("          --p4runtime: generate P4Info message in text format")

def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename

def ignoreStderr(options):
    for line in open(options.p4filename):
        if "P4TEST_IGNORE_STDERR" in line:
            return True
    return False

class Local(object):
    # object to hold local vars accessable to nested functions
    pass

def run_timeout(options, args, timeout, stderr):
    if options.verbose:
        print(args[0], args[len(args) - 1])  # handy for manual cut-and-paste
    print(" ".join(args))
    local = Local()
    local.process = None
    local.filter = None
    def target():
        procstderr = None
        if stderr is not None:
            # copy stderr to the specified file, stripping file path prefixes
            # from the start of lines
            outfile = open(stderr, "w")
            # This regex is ridiculously verbose; it's written this way to avoid
            # features that are not supported on both GNU and BSD (i.e., macOS)
            # sed. BSD sed's character class support is not great; for some
            # reason, even some character classes that the man page claims are
            # available don't seem to actually work.
            local.filter = Popen(['sed', '-E',
                                  r's|^[-[:alnum:][:punct:][:space:]_/]*/([-[:alnum:][:punct:][:space:]_]+\.[ph]4?[:(][[:digit:]]+)|\1|'],
                stdin=PIPE, stdout=outfile)
            procstderr = local.filter.stdin
        local.process = Popen(args, stderr=procstderr)
        local.process.wait()
        if local.filter is not None:
            local.filter.stdin.close()
            local.filter.wait()
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
        print("Comparing", expected, "and", produced)

    cmd = ("diff -B -u -w " + expected + " " + produced + " >&2")
    if options.verbose:
        print(cmd)
    exitcode = subprocess.call(cmd, shell=True);
    if exitcode == 0:
        return SUCCESS
    else:
        return FAILURE

def recompile_file(options, produced, mustBeIdentical):
    # Compile the generated file a second time
    secondFile = produced + "-x";
    args = ["./p4test", "-I.", "--pp", secondFile, "--std", "p4-16", produced] + \
            options.compilerOptions
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
            if result != SUCCESS and (file[-7:] != "-stderr" or not ignoreStderr(options)):
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
    elif "p4_14/" in dirname:
        expected_dirname = dirname.replace("p4_14/", "p4_14_outputs/", 1)
    elif "p4_16/" in dirname:
        expected_dirname = dirname.replace("p4_16/", "p4_16_outputs/", 1)
    else:
        expected_dirname = dirname + "_outputs"  # expected outputs are here
    if not os.path.exists(expected_dirname):
        os.makedirs(expected_dirname)

    # We rely on the fact that these keys are in alphabetical order.
    rename = { "FrontEndDump": "first",
               "FrontEndLast": "frontend",
               "MidEndLast": "midend" }

    if options.verbose:
        print("Writing temporary files into ", tmpdir)
    ppfile = tmpdir + "/" + basename                  # after parsing
    referenceOutputs = ",".join(rename.keys())
    stderr = tmpdir + "/" + basename + "-stderr"
    p4runtimefile = tmpdir + "/" + basename + ".p4info.txt"

    # Create the `json_outputs` directory if it doesn't already exist. There's a
    # race here since multiple tests may run this code in parallel, so we can't
    # check if it exists beforehand.
    try:
        os.mkdir("./json_outputs")
    except OSError as exc:
        if exc.errno == errno.EEXIST:
            pass
        else:
            raise

    jsonfile = "./json_outputs" + "/" + basename + ".json"

    # P4Info generation requires knowledge of the architecture, so we must
    # invoke the compiler with a valid --arch.
    def getArch(path):
        v1Pattern = re.compile('include.*v1model\.p4')
        psaPattern = re.compile('include.*psa\.p4')
        with open(path, 'r') as f:
            for line in f:
                if v1Pattern.search(line):
                    return "v1model"
                elif psaPattern.search(line):
                    return "psa"
            return None

    if not os.path.isfile(options.p4filename):
        raise Exception("No such file " + options.p4filename)
    args = ["./p4test", "--pp", ppfile, "--dump", tmpdir, "--top4", referenceOutputs,
            "--testJson"] + options.compilerOptions
    arch = getArch(options.p4filename)
    if arch is not None:
        args.extend(["--arch", arch])
        if options.generateP4Runtime:
            args.extend(["--p4runtime-files", p4runtimefile])

    if "p4_14" in options.p4filename or "v1_samples" in options.p4filename:
        args.extend(["--std", "p4-14"]);
    args.extend(argv)
    if options.runDebugger:
        args[0:0] = options.runDebugger.split()
        os.execvp(args[0], args)
    result = run_timeout(options, args, timeout, stderr)

    if result != SUCCESS:
        print("Error compiling")
        print("".join(open(stderr).readlines()))
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

    # Canonicalize the generated file names
    lastFile = None

    for k in sorted(rename.keys()):
        files = glob.glob(tmpdir + "/" + base + "*" + k + "*.p4");
        if len(files) > 1:
            print("Multiple files matching", k);
        elif len(files) == 1:
            file = files[0]
            if os.path.isfile(file):
                newName = file_name(tmpdir, base, rename[k], ext)
                os.rename(file, newName)
                lastFile = newName

    if (result == SUCCESS):
        result = check_generated_files(options, tmpdir, expected_dirname);
    if (result == SUCCESS) and (not expected_error):
        result = recompile_file(options, ppfile, False)
    if (result == SUCCESS) and (not expected_error) and (lastFile is not None):
        # Unfortunately compilation and pretty-printing of lastFile is
        # not idempotent: For example a constant such as 8s128 is
        # converted by the compiler to -8s128.
        result = recompile_file(options, lastFile, False)

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
        elif argv[0] == "-j":
            options.dumpToJson = True
        elif argv[0] == "-a":
            if len(argv) == 0:
                print("Missing argument for -a option")
                usage(options)
                sys.exit(FAILURE)
            else:
                options.compilerOptions += argv[1].split();
                argv = argv[1:]
        elif argv[0][1] == 'D' or argv[0][1] == 'I' or argv[0][1] == 'T':
            options.compilerOptions.append(argv[0])
        elif argv[0] == "-gdb":
            options.runDebugger = "gdb --args"
        elif argv[0] == "--p4runtime":
            options.generateP4Runtime = True
        else:
            print("Uknown option ", argv[0], file=sys.stderr)
            usage(options)
            sys.exit(FAILURE)
        argv = argv[1:]

    if 'P4TEST_REPLACE' in os.environ:
        options.replace = True

    options.p4filename=argv[-1]
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcdir):
        options.testName = options.p4filename[len(options.compilerSrcdir):];
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]

    result = process_file(options, argv)
    if isError(options.p4filename) and result == FAILURE:
        print("Program was expected to fail")

    sys.exit(result)

if __name__ == "__main__":
    main(sys.argv)

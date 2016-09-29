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

# Runs the compiler on a sample P4 program generating code for the BMv2
# behavioral model simulator

from __future__ import print_function
from subprocess import Popen
from threading import Thread
import json
import sys
import re
import os
import stat
import tempfile
import shutil
import difflib
import subprocess
import time
import random
import errno
from string import maketrans
try:
    from scapy.layers.all import *
    from scapy.utils import *
except ImportError:
    pass
from bmv2stf import RunBMV2

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
        self.hasBMv2 = False            # Is the behavioral model installed?

def nextWord(text, sep = " "):
    # Split a text at the indicated separator.
    # Note that the separator can be a string.
    # Separator is discarded.
    pos = text.find(sep)
    if pos < 0:
        return text, ""
    l, r = text[0:pos].strip(), text[pos+len(sep):len(text)].strip()
    # print(text, "/", sep, "->", l, "#", r)
    return l, r

class ConfigH(object):
    # Represents an autoconf config.h file
    # fortunately the structure of these files is very constrained
    def __init__(self, file):
        self.file = file
        self.vars = {}
        with open(file) as a:
            self.text = a.read()
        self.ok = False
        self.parse()
    def parse(self):
        while self.text != "":
            self.text = self.text.strip()
            if self.text.startswith("/*"):
                end = self.text.find("*/")
                if end < 1:
                    reportError("Unterminated comment in config file")
                    return
                self.text = self.text[end+2:len(self.text)]
            elif self.text.startswith("#define"):
                define, self.text = nextWord(self.text)
                macro, self.text = nextWord(self.text)
                value, self.text = nextWord(self.text, "\n")
                self.vars[macro] = value
            elif self.text.startswith("#ifndef"):
                junk, self.text = nextWord(self.text, "#endif")
            else:
                reportError("Unexpected text:", self.text)
                return
        self.ok = True
    def __str__(self):
        return str(self.vars)

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

def ByteToHex(byteStr):
    return ''.join( [ "%02X " % ord( x ) for x in byteStr ] ).strip()

def HexToByte(hexStr):
    bytes = []
    hexStr = ''.join( hexStr.split(" ") )
    for i in range(0, len(hexStr), 2):
        bytes.append( chr( int (hexStr[i:i+2], 16 ) ) )
    return ''.join( bytes )

def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename

def reportError(*message):
    print("***", *message)

class Local(object):
    # object to hold local vars accessable to nested functions
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
        reportError("Process failed to start")
        return -1
    if options.verbose:
        print("Exit code ", local.process.returncode)
    return local.process.returncode

timeout = 100

def compare_files(options, produced, expected):
    if options.replace:
        if options.verbose:
            print("Saving new version of ", expected)
        shutil.copy2(produced, expected)
        return SUCCESS

    if options.verbose:
        print("Comparing", produced, "and", expected)
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
        print(message, file=sys.stderr)

    return result

class ConcurrentInteger(object):
    # Generates exclusive integers in a range 0-max
    # in a way which is safe across multiple processes.
    # It uses a simple form of locking using folder names.
    # This is necessary because this script may be invoked
    # concurrently many times by make, and we need the many simulator instances
    # to use different port numbers.
    def __init__(self, folder, max):
        self.folder = folder
        self.max = max
    def lockName(self, value):
        return "lock_" + str(value)
    def release(self, value):
        os.rmdir(self.lockName(value))
    def generate(self):
        # try 10 times
        for i in range(0, 10):
            index = random.randint(0, self.max)
            file = self.lockName(index)
            try:
                os.makedirs(file)
                return index
            except:
                time.sleep(1)
                continue
        return None

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

def run_model(options, tmpdir, jsonfile):
    if not options.hasBMv2:
        return SUCCESS

    # We can do this if an *.stf file is present
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)

    testfile = dirname + "/" + base + ".stf"
    print("Check for ", testfile)
    if not os.path.isfile(testfile):
        return SUCCESS

    bmv2 = RunBMV2(tmpdir, options, jsonfile)
    result = bmv2.generate_model_inputs(testfile)
    if result != SUCCESS:
        return result
    result = bmv2.run()
    if result != SUCCESS:
        return result
    result = bmv2.checkOutputs()
    return result

def process_file(options, argv):
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=".")
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)
    expected_dirname = dirname + "_outputs"  # expected outputs are here

    if options.verbose:
        print("Writing temporary files into ", tmpdir)
    if options.testName:
        jsonfile = options.testName + ".json"
    else:
        jsonfile = tmpdir + "/" + base + ".json"
    stderr = tmpdir + "/" + basename + "-stderr"

    if not os.path.isfile(options.p4filename):
        raise Exception("No such file " + options.p4filename)
    args = ["./p4c-bm2-ss", "-o", jsonfile] + options.compilerOptions
    if "p4_14_samples" in options.p4filename or "v1_samples" in options.p4filename:
        args.extend(["--p4v", "1.0"]);
    args.extend(argv)  # includes p4filename

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

    if result == SUCCESS:
        result = run_model(options, tmpdir, jsonfile);

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

    options.compilerSrcDir = argv[1]
    argv = argv[2:]
    if not os.path.isdir(options.compilerSrcDir):
        print(options.compilerSrcDir + " is not a folder", file=sys.stderr)
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
                reportError("Missing argument for -a option")
                usage(options)
                sys.exit(1)
            else:
                options.compilerOptions += argv[1].split();
                argv = argv[1:]
        elif argv[0][1] == 'D' or argv[0][1] == 'I' or argv[0][1] == 'T':
            options.compilerOptions.append(argv[0]);
        else:
            reportError("Uknown option ", argv[0])
            usage(options)
        argv = argv[1:]

    config = ConfigH("config.h")
    if not config.ok:
        print("Error parsing config.h")
        return FAILURE

    options.hasBMv2 = "HAVE_SIMPLE_SWITCH" in config.vars
    if not options.hasBMv2:
        reportError("config.h indicates that BMv2 is not installed; will skip running BMv2 tests")

    options.p4filename=argv[-1]
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcDir):
        options.testName = options.p4filename[len(options.compilerSrcDir):];
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]
        options.testName = "bmv2/" + options.testName

    result = process_file(options, argv)
    if result != SUCCESS:
        reportError("Test failed")
    sys.exit(result)

if __name__ == "__main__":
    main(sys.argv)

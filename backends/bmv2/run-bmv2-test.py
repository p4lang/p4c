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
import pdb
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
        self.switchOptions = []
        self.switchTargetSpecificOptions = []
        self.hasBMv2 = False            # Is the behavioral model installed?
        self.usePsa = False             # Use the psa switch behavioral model?
        self.runDebugger = False
        self.observationLog = None           # Log packets produced by the BMV2 model if path to log is supplied
        self.initCommands = []

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
    print("          -p: use psa switch")
    print("          -f: replace reference outputs with newly generated ones")
    print("          -a option: pass this option to the compiler")
    print("          --switch-arg option: pass this general option to the switch")
    print("          --target-specific-switch-arg option: pass this target-specific option to the switch")
    print("          -gdb: run compiler under gdb")
    print("          --pp file: pass this option to the compiler")
    print("          -observation-log <file>: save packet output to <file>")
    print("          --init <cmd>: Run <cmd> before the start of the test")


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

timeout = 10 * 60

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
        # If no stf file is present just use the empty file
        testfile = dirname + "/empty.stf"
    if not os.path.isfile(testfile):
        # If no empty.stf present, don't try to run the model at all
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

def run_init_commands(options):
    if not options.initCommands:
        return SUCCESS
    for cmd in options.initCommands:
        args = cmd.split()
        result = run_timeout(options, args, timeout, None)
        if result != SUCCESS:
            return FAILURE
    return SUCCESS

def process_file(options, argv):
    assert isinstance(options, Options)

    if (run_init_commands(options) != SUCCESS):
        return FAILURE
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

    if options.usePsa:
        binary = "./p4c-bm2-psa"
    else:
        binary = "./p4c-bm2-ss"

    args = [binary, "-o", jsonfile] + options.compilerOptions
    if "p4_14" in options.p4filename or "v1_samples" in options.p4filename:
        args.extend(["--std", "p4-14"]);
    args.extend(argv)  # includes p4filename
    if options.runDebugger:
        args[0:0] = options.runDebugger.split()
        os.execvp(args[0], args)
    result = run_timeout(options, args, timeout, stderr)
    #result = SUCCESS

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

    if result == SUCCESS and not expected_error:
        result = run_model(options, tmpdir, jsonfile);

    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result

######################### main

def main(argv):
    options = Options()

    options.binary = argv[0]
    if len(argv) <= 2:
        usage(options)
        sys.exit(FAILURE)

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
        elif argv[0] == "-p":
            options.usePsa = True
        elif argv[0] == "-a":
            if len(argv) == 0:
                reportError("Missing argument for -a option")
                usage(options)
                sys.exit(FAILURE)
            else:
                options.compilerOptions += argv[1].split();
                argv = argv[1:]
        elif argv[0] == "--switch-arg":
            if len(argv) == 0:
                reportError("Missing argument for --switch-arg option")
                usage(options)
                sys.exit(FAILURE)
            else:
                options.switchOptions += argv[1].split();
                argv = argv[1:]
        elif argv[0] == "--target-specific-switch-arg":
            if len(argv) == 0:
                reportError("Missing argument for --target-specific-switch-arg option")
                usage(options)
                sys.exit(FAILURE)
            else:
                options.switchTargetSpecificOptions += argv[1].split();
                argv = argv[1:]
        elif argv[0][1] == 'D' or argv[0][1] == 'I' or argv[0][1] == 'T':
            options.compilerOptions.append(argv[0])
        elif argv[0] == "-gdb":
            options.runDebugger = "gdb --args"
        elif argv[0] == '-observation-log':
            if len(argv) == 0:
                reportError("Missing argument for -observation-log option")
                usage(options)
                sys.exit(FAILURE)
            else:
                options.observationLog = argv[1]
                argv = argv[1:]
        elif argv[0] == "--pp":
            options.compilerOptions.append(argv[0])
            argv = argv[1:]
            options.compilerOptions.append(argv[0])
        elif argv[0] == "--init":
            if len(argv) == 0:
                reportError("Missing argument for --init option")
                usage(options)
                sys.exit(FAILURE)
            else:
                options.initCommands.append(argv[1])
                argv = argv[1:]
        else:
            reportError("Unknown option ", argv[0])
            usage(options)
            sys.exit(FAILURE)
        argv = argv[1:]

    config = ConfigH("config.h")
    if not config.ok:
        print("Error parsing config.h")
        sys.exit(FAILURE)

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

    if not options.observationLog:
        if options.testName:
            options.observationLog = os.path.join('%s.p4.obs' % options.testName)
        else:
            basename = os.path.basename(options.p4filename)
            base, ext = os.path.splitext(basename)
            dirname = os.path.dirname(options.p4filename)
            options.observationLog = os.path.join(dirname, '%s.p4.obs' % base)

    try:
        result = process_file(options, argv)
    except Exception as e:
        print("Exception ", e)
        sys.exit(FAILURE)

    if result != SUCCESS:
        reportError("Test failed")
    sys.exit(result)

if __name__ == "__main__":
    main(sys.argv)

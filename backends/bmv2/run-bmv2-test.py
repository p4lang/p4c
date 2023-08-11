#!/usr/bin/env python3
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

import argparse
import logging
import os
import shutil
import sys
import tempfile
from subprocess import Popen
from threading import Thread

from bmv2stf import RunBMV2
from scapy.layers.all import *
from scapy.utils import *

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)) + "/../../tools")
from testutils import FAILURE, SUCCESS, check_if_dir, check_if_file


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("rootdir", help="the root directory of the compiler source tree")
    parser.add_argument("p4filename", help="the p4 file to process")
    parser.add_argument(
        "-b",
        "--nocleanup",
        action="store_false",
        help="do not remove temporary results for failing tests",
    )
    parser.add_argument(
        "-bd",
        "--buildir",
        dest="builddir",
        help="The path to the compiler build directory, default is \"build\".",
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose operation")
    parser.add_argument(
        "-f",
        "--replace",
        action="store_true",
        help="replace reference outputs with newly generated ones",
    )
    parser.add_argument(
        "-p", "--usePsa", dest="use_psa", action="store_true", help="Use psa switch"
    )
    parser.add_argument("-pp", dest="pp", help="pass this option to the compiler")
    parser.add_argument("-gdb", "--gdb", action="store_true", help="Run the compiler under gdb.")
    parser.add_argument(
        "-a",
        dest="compiler_options",
        default=[],
        action="append",
        nargs="?",
        help="Pass this option string to the compiler",
    )
    parser.add_argument(
        "--target-specific-switch-arg",
        dest="switch_options",
        default=[],
        action="append",
        nargs="?",
        help="Pass this target-specific option to the switch",
    )
    parser.add_argument(
        "--init",
        dest="init_cmds",
        default=[],
        action="append",
        nargs="?",
        help="Run <cmd> before the start of the test",
    )
    parser.add_argument("--observation-log", dest="obs_log", help="save packet output to <file>")
    parser.add_argument(
        "-tf",
        "--testFile",
        dest="testFile",
        help=(
            "Provide the path for the stf file for this test. "
            "If no path is provided, the script will search for an"
            " stf file in the same folder."
        ),
    )
    return parser.parse_known_args()


class Options:
    def __init__(self):
        self.binary = ""  # this program's name
        self.cleanupTmp = True  # if false do not remote tmp folder created
        self.p4filename = ""  # file that is being compiled
        self.compilerSrcDir = ""  # path to compiler source tree
        self.compilerBuildDir = ""  # path to compiler build directory
        self.testFile = ""  # path to stf test file that is used
        self.testName = None  # Name of the test
        self.verbose = False
        self.replace = False  # replace previous outputs
        self.compilerOptions = []
        self.switchTargetSpecificOptions = []
        self.hasBMv2 = False  # Is the behavioral model installed?
        self.usePsa = False  # Use the psa switch behavioral model?
        self.runDebugger = False
        # Log packets produced by the BMV2 model if path to log is supplied
        self.observationLog = None
        self.initCommands = []


def nextWord(text, sep=" "):
    # Split a text at the indicated separator.
    # Note that the separator can be a string.
    # Separator is discarded.
    pos = text.find(sep)
    if pos < 0:
        return text, ""
    l, r = text[0:pos].strip(), text[pos + len(sep) : len(text)].strip()
    # print(text, "/", sep, "->", l, "#", r)
    return l, r


class ConfigH:
    # Represents an autoconf config.h file
    # fortunately the structure of these files is very constrained
    def __init__(self, file):
        self.file = file
        self.vars = {}
        with open(file, "r", encoding="utf-8") as a:
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
                self.text = self.text[end + 2 : len(self.text)]
            elif self.text.startswith("#define"):
                _, self.text = nextWord(self.text)
                macro, self.text = nextWord(self.text)
                value, self.text = nextWord(self.text, "\n")
                self.vars[macro] = value
            elif self.text.startswith("#ifndef"):
                _, self.text = nextWord(self.text, "#endif")
            else:
                reportError("Unexpected text:", self.text)
                return
        self.ok = True

    def __str__(self):
        return str(self.vars)


def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename


def reportError(*message):
    print("***", *message)


class Local:
    # object to hold local vars accessible to nested functions
    process = None


def run_timeout(options, args, timeout, stderr):
    if options.verbose:
        print("Executing ", " ".join(args))
    local = Local()

    def target():
        procstderr = None
        if stderr is not None:
            procstderr = open(stderr, "w", encoding="utf-8")
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
    base, _ = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)

    testFile = options.testFile
    # If no test file is provided, try to find it in the folder.
    if not testFile:
        testFile = dirname + "/" + base + ".stf"
        print("Check for ", testFile)
        if not os.path.isfile(testFile):
            # If no stf file is present just use the empty file
            testFile = dirname + "/empty.stf"
        if not os.path.isfile(testFile):
            # If no empty.stf present, don't try to run the model at all
            return SUCCESS
    bmv2 = RunBMV2(tmpdir, options, jsonfile)

    stf_map, result = bmv2.parse_stf_file(testFile)
    if result != SUCCESS:
        return result

    result = bmv2.generate_model_inputs(stf_map)
    if result != SUCCESS:
        return result
    result = bmv2.run(stf_map)
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

    if run_init_commands(options) != SUCCESS:
        return FAILURE
    tmpdir = tempfile.mkdtemp(dir=".")
    basename = os.path.basename(options.p4filename)
    base, _ = os.path.splitext(basename)

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
        binary = options.compilerBuildDir + "/p4c-bm2-psa"
    else:
        binary = options.compilerBuildDir + "/p4c-bm2-ss"

    args = [binary, "-o", jsonfile] + options.compilerOptions
    if "p4_14" in options.p4filename or "v1_samples" in options.p4filename:
        args.extend(["--std", "p4-14"])
    args.append(options.p4filename)
    args.extend(argv)
    if options.runDebugger:
        args[0:0] = options.runDebugger.split()
        os.execvp(args[0], args)
    result = run_timeout(options, args, timeout, stderr)

    if result != SUCCESS:
        print("Error compiling")
        with open(stderr, mode="r", encoding="utf-8") as stderr_file:
            print(stderr_file.read())
            # If the compiler crashed fail the test
            if "Compiler Bug" in stderr_file.read():
                return FAILURE

    expected_error = isError(options.p4filename)
    if expected_error:
        # invert result
        if result == SUCCESS:
            result = FAILURE
        else:
            result = SUCCESS

    if result == SUCCESS and not expected_error:
        result = run_model(options, tmpdir, jsonfile)

    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result


def main(argv):
    try:
        result = process_file(options, argv)
    except Exception as e:
        print(f"There was a problem executing the STF test {options.testFile}:")
        raise e

    if result != SUCCESS:
        reportError("Test failed")
    sys.exit(result)


if __name__ == "__main__":
    # Parse options and process argv
    args, argv = parse_args()
    options = Options()
    options.binary = ""
    # TODO: Convert these paths to pathlib's Path.
    options.p4filename = check_if_file(args.p4filename).as_posix()
    options.compilerSrcDir = check_if_dir(args.rootdir).as_posix()

    # If no build directory is provided, append build to the compiler src dir.
    if args.builddir:
        options.compilerBuildDir = args.builddir
    else:
        options.compilerBuildDir = options.compilerSrcDir + "/build"
    options.verbose = args.verbose
    options.replace = args.replace
    options.cleanupTmp = args.nocleanup
    options.testFile = args.testFile

    # We have to be careful here, passing compiler options is ambiguous in
    # argparse because the parser is not positional.
    # https://stackoverflow.com/a/21894384/3215972
    # For each list option, such as compiler_options,
    # we use -a="--compiler-arg" instead.
    for compiler_option in args.compiler_options:
        options.compilerOptions.extend(compiler_option.split())
    for switch_option in args.switch_options:
        options.switchTargetSpecificOptions.extend(switch_option.split())
    for init_cmd in args.init_cmds:
        options.initCommands.append(init_cmd)
    options.usePsa = args.use_psa
    if args.pp:
        options.compilerOptions.append(args.pp)
    if args.gdb:
        options.runDebugger = "gdb --args"
    options.observationLog = args.obs_log
    residual_argv = []
    for arg in argv:
        if arg in ("-D", "-I", "-T"):
            options.compilerOptions.append(arg)
        else:
            residual_argv.append(arg)

    config = ConfigH(options.compilerBuildDir + "/config.h")
    if not config.ok:
        print("Error parsing config.h")
        sys.exit(FAILURE)

    # Configure logging.
    logging.basicConfig(
        filename="test.log",
        format="%(levelname)s: %(message)s",
        level=getattr(logging, "INFO"),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    logging.getLogger().addHandler(stderr_log)

    options.hasBMv2 = "HAVE_SIMPLE_SWITCH" in config.vars
    if not options.hasBMv2:
        reportError("config.h indicates that BMv2 is not installedwill skip running BMv2 tests")
    if options.p4filename.startswith(options.compilerBuildDir):
        options.testName = options.p4filename[len(options.compilerBuildDir) :]
        if options.testName.startswith("/"):
            options.testName = options.testName[1:]
        if options.testName.endswith(".p4"):
            options.testName = options.testName[:-3]
        options.testName = "bmv2/" + options.testName
    if not options.observationLog:
        if options.testName:
            options.observationLog = os.path.join(f"{options.testName}.p4.obs")
        else:
            basename = os.path.basename(options.p4filename)
            base, _ = os.path.splitext(basename)
            dirname = os.path.dirname(options.p4filename)
            options.observationLog = os.path.join(dirname, f"{base}.p4.obs")

    # All args after '--' are intended for the p4 compiler
    main(residual_argv)

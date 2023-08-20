#!/usr/bin/env python3
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

import argparse
import logging
import os
import sys
import tempfile
from pathlib import Path

FILE_DIR = Path(__file__).resolve().parent
sys.path.append(str(FILE_DIR.joinpath("../../tools")))
import testutils

PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir", help="the root directory of the compiler source tree")
PARSER.add_argument("p4filename", help="the p4 file to process")
PARSER.add_argument(
    "-b",
    "--nocleanup",
    action="store_false",
    help="do not remove temporary results for failing tests",
)
PARSER.add_argument(
    "-c",
    "--compiler",
    dest="compiler",
    default="p4c-ebpf",
    help="Specify the path to the compiler binary, default is p4c-ebpf",
)
PARSER.add_argument(
    "-f",
    "--replace",
    action="store_true",
    help="replace reference outputs with newly generated ones",
)
PARSER.add_argument(
    "-t",
    "--target",
    dest="target",
    default="test",
    help="Specify the compiler backend target, default is test",
)
PARSER.add_argument(
    "-e",
    "--extern-file",
    dest="extern",
    default="",
    help="Specify path additional file with C extern function definition",
)
PARSER.add_argument(
    "-tf",
    "--testfile",
    dest="testfile",
    help=(
        "Provide the path for the stf file for this test. "
        "If no path is provided, the script will search for an"
        " stf file in the same folder."
    ),
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="WARNING",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)


def import_from(module, name):
    """Try to import a module and class directly instead of the typical
    Python method. Allows for dynamic imports."""
    module = __import__(module, fromlist=[name])
    return getattr(module, name)


class EBPFFactory:
    """Generator class.
    Returns a target subclass based on the provided target option."""

    @staticmethod
    def create(tmpdir, options, template):
        target_name = "targets." + options.target + "_target"
        target_class = "Target"
        testutils.log.info(f"Loading target {target_name}")
        try:
            EBPFTarget = import_from(target_name, target_class)
        except ImportError as e:
            testutils.log.error(e)
            return None
        return EBPFTarget(tmpdir, options, template)


class Options:
    def __init__(self):
        self.binary = ""  # This program's name.
        self.cleanupTmp = True  # Remove tmp folder?
        self.compiler = ""  # Path to the P4 compiler binary.
        self.p4filename = ""  # File that is being compiled.
        self.testfile = ""  # path to stf test file that is used.
        self.replace = False  # Replace previous outputs.
        self.target = "test"  # The name of the target compiler.
        # Actual location of the test framework.
        self.testdir = str(FILE_DIR)
        # The location of the eBPF runtime, some targets may overwrite this.
        self.runtimedir = str(FILE_DIR.joinpath("runtime"))
        self.extern = ""  # Path to C file with extern definition.


def run_model(ebpf, testfile):
    result = ebpf.generate_model_inputs(testfile)
    if result != testutils.SUCCESS:
        return result

    result = ebpf.compile_dataplane()
    if result != testutils.SUCCESS:
        return result

    result = ebpf.run()
    if result == testutils.SKIPPED:
        return testutils.SUCCESS
    if result != testutils.SUCCESS:
        return result

    result = ebpf.check_outputs()
    return result


def run_test(options, argv):
    """Define the test environment and compile the p4 target
    Optional: Run the generated model"""
    assert isinstance(options, Options)

    basename = os.path.basename(options.p4filename)  # Name of the p4 test
    base, _ = os.path.splitext(basename)  # Name without the type
    dirname = os.path.dirname(options.p4filename)  # Directory of the file

    testfile = options.testfile
    # If no test file is provided, try to find it in the folder.
    if not testfile:
        # We can do this if an *.stf file is present
        testfile = dirname + "/" + base + ".stf"
        testutils.log.info(f"Checking for {testfile}")
        if not os.path.isfile(testfile):
            # If no stf file is present just use the empty file
            testfile = dirname + "/empty.stf"

    template = options.testdir + "/" + "test"
    testutils.log.info(f"Writing temporary files into {options.testdir}")

    ebpf = EBPFFactory.create(options.testdir, options, template)
    if ebpf is None:
        return testutils.FAILURE

    # If extern file is passed, --emit-externs flag is added by default to the p4 compiler
    if options.extern:
        argv.append("--emit-externs")
    # Compile the p4 file to the specified target
    result, expected_error = ebpf.compile_p4(argv)

    # Compile and run the generated output
    if result == testutils.SUCCESS and not expected_error:
        # If no empty.stf present, don't try to run the model at all
        if not os.path.isfile(testfile):
            msg = "No stf file present!"
            testutils.log.info(msg)
            result = testutils.SUCCESS
        else:
            result = run_model(ebpf, testfile)
    # Only if we did not expect it to fail
    if result != testutils.SUCCESS:
        return result

    # Remove the tmp folder
    if options.cleanupTmp:
        testutils.del_dir(options.testdir)
    return result


if __name__ == "__main__":
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()
    options = Options()
    # TODO: Convert these paths to pathlib's Path.
    options.compiler = testutils.check_if_file(args.compiler).as_posix()
    options.p4filename = testutils.check_if_file(args.p4filename).as_posix()
    options.replace = args.replace
    options.cleanupTmp = args.nocleanup
    if args.testfile:
        options.testfile = testutils.check_if_file(args.testfile).as_posix()
    options.target = args.target
    options.extern = args.extern
    options.testdir = tempfile.mkdtemp(dir=os.path.abspath("./"))
    os.chmod(options.testdir, 0o755)
    # Configure logging.
    logging.basicConfig(
        filename=options.testdir + "/testlog.log",
        format="%(levelname)s:%(message)s",
        level=getattr(logging, args.log_level),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s:%(message)s"))
    logging.getLogger().addHandler(stderr_log)

    # All args after '--' are intended for the p4 compiler
    argv = argv[1:]
    # Run the test with the extracted options and modified argv
    result = run_test(options, argv)
    sys.exit(result)

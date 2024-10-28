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
import sys
import tempfile
import traceback
from pathlib import Path
from typing import Any, Dict, Optional, Tuple

PARSER = argparse.ArgumentParser()
PARSER.add_argument(
    "rootdir",
    help="The root directory of the compiler source tree."
    "This is used to import P4C's Python libraries",
)
PARSER.add_argument("p4_file", help="the p4 file to process")
PARSER.add_argument(
    "-tf",
    "--test_file",
    dest="test_file",
    help=(
        "Provide the path for the stf file for this test. "
        "If no path is provided, the script will search for an"
        " stf file in the same folder."
    ),
)
PARSER.add_argument(
    "-td",
    "--testdir",
    dest="testdir",
    help="The location of the test directory.",
)
PARSER.add_argument(
    "-b",
    "--nocleanup",
    action="store_true",
    dest="nocleanup",
    help="Do not remove temporary results for failing tests.",
)
PARSER.add_argument(
    "-n",
    "--num-ifaces",
    default=8,
    dest="num_ifaces",
    help="How many virtual interfaces to create.",
)
PARSER.add_argument(
    "-nn",
    "--use-nanomsg",
    action="store_true",
    dest="use_nn",
    help="Use nanomsg for packet sending instead of virtual interfaces.",
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="WARNING",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)
PARSER.add_argument(
    "-f",
    "--replace",
    action="store_true",
    help="replace reference outputs with newly generated ones",
)
PARSER.add_argument("-p", "--use_psa", dest="use_psa", action="store_true", help="Use psa switch")
PARSER.add_argument(
    "-bd",
    "--buildir",
    dest="builddir",
    help="The path to the compiler build directory, default is current directory.",
)
PARSER.add_argument("-pp", dest="pp", help="pass this option to the compiler")
PARSER.add_argument("-gdb", "--gdb", action="store_true", help="Run the compiler under gdb.")
PARSER.add_argument("-lldb", "--lldb", action="store_true", help="Run the compiler under lldb.")
PARSER.add_argument(
    "-a",
    dest="compiler_options",
    default=[],
    action="append",
    nargs="?",
    help="Pass this option string to the compiler",
)
PARSER.add_argument(
    "--switch-arg",
    dest="switch_options",
    default=[],
    action="append",
    nargs="?",
    help="Pass this option to the switch",
)
PARSER.add_argument(
    "--target-specific-switch-arg",
    dest="switch_target_options",
    default=[],
    action="append",
    nargs="?",
    help="Pass this target-specific option to the switch",
)
PARSER.add_argument(
    "--init",
    dest="init_cmds",
    default=[],
    action="append",
    nargs="?",
    help="Run <cmd> before the start of the test",
)
PARSER.add_argument("--observation-log", dest="obs_log", help="save packet output to <file>")


# Parse options and process argv
ARGS, ARGV = PARSER.parse_known_args()

# Append the root directory to the import path.
FILE_DIR = Path(__file__).resolve().parent
ROOT_DIR = Path(ARGS.rootdir).absolute()
sys.path.append(str(ROOT_DIR))

from bmv2stf import Options, RunBMV2
from scapy.layers.all import *
from scapy.utils import *

from tools import testutils  # pylint: disable=wrong-import-position


def nextWord(text: str, sep: str = " ") -> Tuple[str, str]:
    # Split a text at the indicated separator.
    # Note that the separator can be a string.
    # Separator is discarded.
    pos = text.find(sep)
    if pos < 0:
        return text, ""
    l, r = text[0:pos].strip(), text[pos + len(sep) : len(text)].strip()
    # testutils.log.info(text, "/", sep, "->", l, "#", r)
    return l, r


class ConfigH:
    # Represents an autoconf config.h file
    # fortunately the structure of these files is very constrained
    def __init__(self, file: Path) -> None:
        self.file = file
        self.vars: Dict[str, str] = {}
        with open(file, "r", encoding="utf-8") as a:
            self.text = a.read()
        self.ok = False
        self.parse()

    def parse(self) -> None:
        while self.text != "":
            self.text = self.text.strip()
            if self.text.startswith("/*"):
                end = self.text.find("*/")
                if end < 1:
                    testutils.log.error("Unterminated comment in config file")
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
                testutils.log.error("Unexpected text:", self.text)
                return
        self.ok = True

    def __str__(self) -> str:
        return str(self.vars)


def isError(p4_file: Path) -> bool:
    # True if the filename represents a p4 program that should fail
    return "_errors" in str(p4_file)


def run_model(options: Options, tmpdir: Path, jsonfile: Path) -> int:
    if not options.test_file:
        return testutils.SUCCESS
    bmv2 = RunBMV2(tmpdir, options, jsonfile)

    stf_map, result = bmv2.parse_stf_file(options.test_file)
    if result != testutils.SUCCESS:
        return result

    result = bmv2.generate_model_inputs(stf_map)
    if result != testutils.SUCCESS:
        return result

    if not options.has_bmv2:
        testutils.log.error(
            "config.h indicates that BMv2 is not installed. Will skip running BMv2 tests"
        )
        return testutils.SUCCESS

    result = bmv2.run(stf_map)
    if result != testutils.SUCCESS:
        return result
    result = bmv2.checkOutputs()
    return result


def run_init_commands(options: Options) -> int:
    if not options.init_commands:
        return testutils.SUCCESS
    for cmd in options.init_commands:
        result = testutils.exec_process(cmd, timeout=30)
        if result.returncode != testutils.SUCCESS:
            return testutils.FAILURE
    return testutils.SUCCESS


def process_file(options: Options) -> int:
    assert isinstance(options, Options)

    if run_init_commands(options) != testutils.SUCCESS:
        return testutils.FAILURE
    # Ensure that tempfile.mkdtemp returns an absolute path, regardless of the py3 version.
    tmpdir = Path(tempfile.mkdtemp(dir=options.testdir.absolute()))
    base = options.p4_file.stem

    testutils.log.debug("Writing temporary files into ", tmpdir)
    if options.testName:
        jsonfile = Path(options.testName).with_suffix(".json")
    else:
        jsonfile = tmpdir.joinpath(base).with_suffix(".json")

    if not options.p4_file.is_file():
        raise Exception(f"No such file {options.p4_file}")

    if options.use_psa:
        binary = options.compiler_build_dir.joinpath("p4c-bm2-psa")
    else:
        binary = options.compiler_build_dir.joinpath("p4c-bm2-ss")

    cmd: str = f"{binary} -o {jsonfile}"
    for opt in options.compiler_options:
        cmd += f" {opt}"
    if "p4_14" in str(options.p4_file) or "v1_samples" in str(options.p4_file):
        cmd += " --std p4-14"
    cmd += f" {options.p4_file}"
    if options.run_debugger:
        debugger_cmd = f"{options.run_debugger} {cmd}".split()
        os.execvp(debugger_cmd[0], debugger_cmd)
        # We terminate after running the debugger.
        return testutils.SUCCESS
    result = testutils.exec_process(cmd, timeout=30)
    returnvalue = result.returncode
    if returnvalue != testutils.SUCCESS:
        testutils.log.error("Error compiling")
        # If the compiler crashed fail the test
        if "Compiler Bug" in result.output:
            return testutils.FAILURE

    expected_error = isError(options.p4_file)
    if expected_error:
        # invert result
        if returnvalue == testutils.SUCCESS:
            returnvalue = testutils.FAILURE
        else:
            returnvalue = testutils.SUCCESS

    if returnvalue == testutils.SUCCESS and not expected_error:
        return run_model(options, tmpdir, jsonfile)

    return returnvalue


def run_test(options: Options) -> int:
    try:
        result = process_file(options)
    except Exception as e:
        testutils.log.error(f"There was a problem executing the STF test {options.test_file}:\n{e}")
        testutils.log.error(traceback.format_exc())
        return testutils.FAILURE

    if result != testutils.SUCCESS:
        testutils.log.error("Test failed")
    return result


def create_options(test_args: Any) -> Optional[Options]:
    """Parse the input arguments and create a processed options object."""
    options = Options()
    testdir = test_args.testdir
    if not testdir:
        # FIXME: This is logging before logging is set up...
        testutils.log.info("No test directory provided. Generating temporary folder.")
        testdir = tempfile.mkdtemp(dir=Path(".").absolute())
        # Generous permissions because the program is usually edited by sudo.
        os.chmod(testdir, 0o755)
    options.testdir = Path(testdir)
    # Configure logging.
    logging.basicConfig(
        filename=options.testdir.joinpath("test.log"),
        format="%(levelname)s: %(message)s",
        level=getattr(logging, test_args.log_level),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    logging.getLogger().addHandler(stderr_log)

    result = testutils.check_if_file(test_args.p4_file)
    if not result:
        return None
    options.p4_file = result
    test_file = test_args.test_file
    if not test_file:
        testutils.log.info("No test file provided. Checking for file in folder.")
        test_file = options.p4_file.with_suffix(".stf")
    result = testutils.check_if_file(test_file)
    if testutils.check_if_file(test_file):
        testutils.log.info(f"Using test file {test_file}")
        options.test_file = Path(test_file)
    else:
        options.test_file = None
    options.rootdir = Path(test_args.rootdir)

    # If no build directory is provided, use current working directory
    if test_args.builddir:
        options.compiler_build_dir = Path(test_args.builddir)
    else:
        options.compiler_build_dir = Path.cwd()
    options.replace = test_args.replace
    options.cleanupTmp = test_args.nocleanup

    # We have to be careful here, passing compiler options is ambiguous in
    # argparse because the parser is not positional.
    # https://stackoverflow.com/a/21894384/3215972
    # For each list option, such as compiler_options,
    # we use -a="--compiler-arg" instead.
    for compiler_option in test_args.compiler_options:
        options.compiler_options.extend(compiler_option.split())
    for switch_option in test_args.switch_options:
        options.switch_options.extend(switch_option.split())
    for switch_option in test_args.switch_target_options:
        options.switch_target_options.extend(switch_option.split())
    for init_cmd in test_args.init_cmds:
        options.init_commands.append(init_cmd)
    options.use_psa = test_args.use_psa
    if test_args.pp:
        options.compiler_options.append(test_args.pp)
    if test_args.gdb:
        options.run_debugger = "gdb --args "
    if test_args.lldb:
        options.run_debugger = "lldb -- "
    options.observation_log = test_args.obs_log
    config = ConfigH(options.compiler_build_dir.joinpath("config.h"))
    if not config.ok:
        testutils.log.info("Error parsing config.h")
        return None

    options.has_bmv2 = "HAVE_SIMPLE_SWITCH" in config.vars
    if options.compiler_build_dir in options.p4_file.parents:
        options.testName = str(options.p4_file.relative_to(options.compiler_build_dir))
        if options.testName.startswith("/"):
            options.testName = options.testName[1:]
        if options.testName.endswith(".p4"):
            options.testName = options.testName[:-3]
        options.testName = "bmv2/" + options.testName
    if not options.observation_log:
        if options.testName:
            options.observation_log = Path(options.testName).with_suffix(".p4.obs")
        else:
            basename = os.path.basename(options.p4_file)
            base, _ = os.path.splitext(basename)
            dirname = options.p4_file.parent
            options.observation_log = dirname.joinpath(f"{base}.p4.obs")
    return options


if __name__ == "__main__":
    test_options = create_options(ARGS)
    if not test_options:
        sys.exit(testutils.FAILURE)

    # Run the test with the extracted options
    test_result = run_test(test_options)
    if not (ARGS.nocleanup or test_result != testutils.SUCCESS):
        testutils.log.info("Removing temporary test directory.")
        testutils.del_dir(test_options.testdir)
    sys.exit(test_result)

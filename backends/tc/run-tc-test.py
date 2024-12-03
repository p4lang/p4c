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

import argparse
import logging
import os
import re
import sys
import tempfile
from pathlib import Path
from typing import Any, Optional

from test_infra import TCInfra

FILE_DIR = Path(__file__).resolve().parent
sys.path.append(str(FILE_DIR.joinpath("../../tools")))
import testutils

PARSER = argparse.ArgumentParser()
PARSER.add_argument("compiler_src_dir", help="the root directory of the compiler source tree")
PARSER.add_argument("p4filename", help="the p4 file to process")
PARSER.add_argument(
    "-c",
    "--compiler",
    dest="compiler",
    default="p4c-pna-p4tc",
    help="Specify the path to the compiler binary, default is p4c-pna-p4tc",
)
PARSER.add_argument(
    "-cb",
    "--clang-bin",
    dest="clang",
    action='store_true',
    default="clang-15",
    help="Specify clang binary, default is clang-15",
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
    "-v",
    "--verbose",
    dest="verbose",
    action='store_true',
    help=("Verbose output"),
)
PARSER.add_argument(
    "-b",
    dest="cleanupTmp",
    action='store_true',
    help=("Do not remove temporary results for failing tests"),
)
PARSER.add_argument(
    "-f",
    "--replace",
    dest="replace",
    action='store_true',
    help=("Replace"),
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="DEBUG",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)


class Options(object):
    def __init__(self) -> None:
        self.binary = ""  # this program's name
        self.cleanupTmp = True  # if false do not remote tmp folder created
        self.p4Filename = ""  # file that is being compiled
        self.compiler_src_dir = ""  # path to compiler source tree
        self.verbose = False
        self.replace = False  # replace previous outputs
        self.compiler = ""
        self.clang = ""
        self.p4filename = ""
        self.testfile: Optional[Path] = None
        self.testdir: Optional[Path] = None
        self.runtimedir = str(FILE_DIR.joinpath("runtime"))
        self.compilerOptions = []


def run_model(tc: TCInfra, testfile: Optional[Path]) -> int:
    result = tc.compile_p4()
    if result != testutils.SUCCESS:
        return result

    result = tc.compare_compiled_files()
    if result != testutils.SUCCESS:
        return result

    # If there is no testfile, just run the compilation testing
    if not testfile:
        return result

    result = tc.spawn_bridge()
    if result != testutils.SUCCESS:
        return result

    result = tc.boot()
    if result != testutils.SUCCESS:
        return result

    result = tc.run(testfile)
    if result != testutils.SUCCESS:
        return result

    result = tc.check_outputs()
    if result != testutils.SUCCESS:
        return result

    return result


def run_test(options: Options, argv: Any) -> int:
    """Define the test environment and compile the p4 target
    Optional: Run the generated model"""
    assert isinstance(options, Options)

    template, _ = os.path.splitext(options.p4filename)
    tmpdir = tempfile.mkdtemp(dir=Path(FILE_DIR.joinpath("runtime")).absolute())
    outputfolder = tmpdir

    tc = TCInfra(outputfolder, options, template)

    return run_model(tc, options.testfile)


######################### main


def clang_is_installed(options: Options) -> bool:
    result = testutils.exec_process(f"{options.clang} --version")
    if result.returncode != testutils.SUCCESS:
        testutils.log.error(f"{options.clang} is not installed")
        return False

    version_pattern = r'clang version (\d+)\.\d+\.\d+'

    match = re.search(version_pattern, result.output)
    if match:
        version = match.group(1)
    else:
        testutils.log.error(f"Not able to retrieve {options.clang} version")
        return False

    if int(version) < 15:
        testutils.log.error(f"{options.clang} version")
        return False

    return True


def main(argv: Any) -> None:
    args, argv = PARSER.parse_known_args()
    options = Options()
    options.compiler_src_dir = testutils.check_if_dir(Path(args.compiler_src_dir))
    compiler = "./" + args.compiler

    options.compiler = testutils.check_if_file(Path(compiler))
    options.clang = args.clang
    if not clang_is_installed(options):
        sys.exit(1)

    options.p4filename = testutils.check_if_file(Path(args.p4filename))
    options.replace = args.replace
    if args.testfile:
        options.testfile = testutils.check_if_file(Path(args.testfile))
    options.testdir = Path(tempfile.mkdtemp(dir=os.path.abspath("./")))

    # Configure logging.
    logging.basicConfig(
        filename=options.testdir.joinpath("test.log"),
        format="%(levelname)s: %(message)s",
        level=getattr(logging, args.log_level),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    logging.getLogger().addHandler(stderr_log)

    if args.cleanupTmp:
        options.cleanupTmp = False

    if args.verbose:
        options.verbose = True

    if args.replace:
        options.replace = True

    argv = argv[1:]

    if "P4TEST_REPLACE" in os.environ:
        options.replace = True

    result = run_test(options, argv)

    sys.exit(result)


if __name__ == "__main__":
    main(sys.argv)

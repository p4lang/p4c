#!/usr/bin/env python3

# Copyright 2019 Orange
# SPDX-FileCopyrightText: 2019 Orange
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import logging
import os
import sys
import tempfile
from pathlib import Path

FILE_DIR = Path(__file__).resolve().parent
sys.path.append(str(FILE_DIR.joinpath("../../tools")))
# path to the framework repository of the compiler
sys.path.append(str(FILE_DIR.joinpath("../ebpf")))
import testutils

run_ebpf_test = importlib.import_module("run-ebpf-test")

arg_parser = run_ebpf_test.PARSER

if __name__ == "__main__":
    # Parse options and process argv
    args, argv = arg_parser.parse_known_args()
    options = run_ebpf_test.Options()
    # TODO: Convert these paths to pathlib's Path.
    options.compiler = testutils.check_if_file(args.compiler).as_posix()
    options.p4filename = testutils.check_if_file(args.p4filename).as_posix()
    options.replace = args.replace
    options.cleanupTmp = args.nocleanup
    options.target = args.target
    options.extern = args.extern
    # Switch test directory based on path to run-ubpf-test.py
    options.runtimedir = str(FILE_DIR.joinpath("runtime"))
    options.testdir = tempfile.mkdtemp(dir=os.path.abspath("./"))
    os.chmod(options.testdir, 0o744)
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
    result = run_ebpf_test.run_test(options, argv)
    sys.exit(result)

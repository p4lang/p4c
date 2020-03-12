#!/usr/bin/env python3
# Copyright 2019 Orange
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

import sys
import os
import importlib

# path to the framework repository of the compiler
sys.path.insert(0, os.path.dirname(
    os.path.realpath(__file__)) + '/../ebpf')
run_ebpf_test = importlib.import_module('run-ebpf-test')

arg_parser = run_ebpf_test.PARSER

if __name__ == "__main__":
    # Parse options and process argv
    args, argv = arg_parser.parse_known_args()
    options = run_ebpf_test.Options()
    options.compiler = run_ebpf_test.check_path(args.compiler)
    options.p4filename = run_ebpf_test.check_path(args.p4filename)
    options.verbose = args.verbose
    options.replace = args.replace
    options.cleanupTmp = args.nocleanup
    options.target = args.target
    # Switch test directory based on path to run-ubpf-test.py
    options.testdir = os.path.dirname(os.path.realpath(__file__))
    options.extern = args.extern

    # All args after '--' are intended for the p4 compiler
    argv = argv[1:]
    # Run the test with the extracted options and modified argv
    result = run_ebpf_test.run_test(options, argv)
    sys.exit(result)

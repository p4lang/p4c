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

import argparse
from subprocess import Popen
from pathlib import Path
import sys
import os

SUCCESS = 0
FAILURE = 1
SKIPPED = 999

PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir", help="the root directory of "
                                    "the compiler source tree")
PARSER.add_argument("p4filepath", help="path to the p4 file to process")
PARSER.add_argument("-c", "--compiler", dest="compiler", default="p4c-ubpf",
                    help="Specify the path to the compiler binary, "
                         "default is p4c-ubpf")


class Options(object):
    def __init__(self):
        self.compiler = ""  # Path to the P4 compiler binary
        self.p4filepath = ""  # Absolute file path that is being compiled
        self.p4filename = ""  # File name that is being compiled
        self.testdir = ""  # Absolute path to tests working directory
        self.referencedir = ""  # Absolute path to directory with reference p4c-ubpf outputs


class UbpfTest(object):

    def __init__(self, options):
        self.options = options

    def run(self):
        result = self.compile_file()
        if result != SUCCESS:
            sys.exit(result)

        result = self.compare_output()
        if result == SKIPPED:
            sys.exit(SUCCESS)
        if result != SUCCESS:
            sys.exit(result)

        sys.exit(SUCCESS)

    def compile_file(self):
        """
        Compiles P4 program.
        :return: True if success.
        """
        cmd = [self.options.compiler, self.options.p4filepath, '-o', self.options.cfilepath]

        procstderr = open(self.options.stderrpath, "w")
        procstdout = open(self.options.stdoutpath, "w")
        proc = Popen(cmd, stderr=procstderr, stdout=procstdout)
        output, error = proc.communicate()

        if error:
            print("Error while compiling: %s" % str(error))
            return FAILURE
        return SUCCESS

    def compare_output(self):
        """
        Compares generated output from p4c-ubpf compiler with reference C programs.
        :return: SUCCESS if no differences.
        """

        reference_c_filepath = os.path.join(options.referencedir, options.cfilename)
        c_status = self.compare(options.cfilepath, reference_c_filepath)

        reference_h_filepath = os.path.join(options.referencedir, options.cfilename).replace(".c", ".h")
        compiled_h_filepath = options.cfilepath.replace(".c", ".h")
        h_status = self.compare(compiled_h_filepath, reference_h_filepath)

        if c_status == SKIPPED or h_status == SKIPPED:
            return SKIPPED

        if c_status != SUCCESS or h_status != SUCCESS:
            return FAILURE
        return SUCCESS

    def compare(self, compiled_file, reference_file):
        with open(compiled_file, 'r') as cf:
            compiled_lines = [line.rstrip() for line in cf.readlines()]
        with open(reference_file, 'r') as rf:
            expected_lines = [line.rstrip() for line in rf.readlines()]

        if not compiled_lines:
            print("Cannot compare files, compiled file is empty. Skipping..")
            return SKIPPED
        if not expected_lines:
            print("Cannot compare files, reference file is empty. Skipping..")
            return SKIPPED

        differences = self.extract_differences(compiled_lines, expected_lines)
        if differences:
            print("The compiled file does not match the reference file:")
            for line in differences:
                print(line)
            with open(self.options.stderrpath, 'a') as file:
                file.write("Output file (%s) does not match the reference file (%s) \n" %
                           (compiled_file, reference_file))
                file.writelines(differences)
            return FAILURE

        return SUCCESS

    def extract_differences(self, compiled_lines, expected_lines):
        differences = []
        expected_nr_of_lines = len(expected_lines)
        is_line_comment = False
        for i in range(0, expected_nr_of_lines):
            if compiled_lines[i].startswith("/*") and expected_lines[i].startswith("/*"):
                is_line_comment = True
                continue
            elif compiled_lines[i].rstrip().endswith("*/") and expected_lines[i].rstrip().endswith("*/"):
                is_line_comment = False
                continue
            elif is_line_comment:
                continue
            if compiled_lines[i] != expected_lines[i]:
                differences.append("Compiled (line %d): %s" % (i+1, compiled_lines[i]))
                differences.append("Expected (line %d): %s" % (i+1, expected_lines[i]))
                break
        return differences


def check_path(path):
    """Checks if a path is an actual directory and converts the input
        to an absolute path"""
    if not os.path.exists(path):
        msg = "{0} does not exist".format(path)
        raise argparse.ArgumentTypeError(msg)
    else:
        return os.path.abspath(os.path.expanduser(path))


if __name__ == "__main__":
    args, argv = PARSER.parse_known_args()
    options = Options()
    options.compiler = check_path(args.compiler)
    options.p4filepath = check_path(args.p4filepath)
    (p4filedir, p4filename) = os.path.split(args.p4filepath)
    options.p4filename = p4filename
    options.cfilename = options.p4filename.replace(".p4", ".c")
    options.referencedir = p4filedir + "_outputs"
    options.testdir = args.rootdir + "/build/ubpf/tests"
    Path(options.testdir).mkdir(parents=True, exist_ok=True)
    options.cfilepath = os.path.join(options.testdir, options.cfilename)
    options.stderrpath = os.path.join(options.testdir, options.p4filename + "-stderr")
    options.stdoutpath = os.path.join(options.testdir, options.p4filename + "-stdout")

    ubpf_test = UbpfTest(options)
    ubpf_test.run()

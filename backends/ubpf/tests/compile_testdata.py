#!/usr/bin/env python

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

import os
import subprocess
from os import listdir


def compile_p4_to_c(filename):
    file = filename + ".p4"
    p4_file_path = os.path.join("testdata", file)
    output_file_path = os.path.join("build", filename + ".c")

    if os.path.exists(p4_file_path):
        print("Compiling %s.p4 ..." % filename)
        cmd = [
            "p4c-ubpf",
            "-o",
            output_file_path,
            p4_file_path,
            "-vvv",
            "-I../p4include",
        ]
        print("Command: ", " ".join(str(x) for x in cmd))
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = proc.communicate()

        if error and "error" in error:
            print("Error: ", error)
            raise Exception(error)

        if output:
            print("Output: ", output)

    else:
        print("File %s was not found under testdata folder" % filename)
        return


def compile_c_to_o(filename):
    print("Compiling %s.c ..." % filename)
    file = filename + ".c"
    c_file_path = os.path.join("build", file)
    output_file_path = os.path.join("build", filename + ".o")
    cmd = [
        "$CLANG_DIR/build/bin/clang",
        "-O2",
        "-target",
        "bpf",
        "-c",
        c_file_path,
        "-o",
        output_file_path,
        "-I../runtime",
    ]
    print("Command: ", " ".join(str(x) for x in cmd))
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = proc.communicate()

    if output:
        print("Output: ", output)

    if error:
        print("Error: ", error)


def main():
    print("Creating build directory ...")

    if not os.path.exists("build"):
        os.makedirs("build")

    print("Starting compiling P4 programs...")

    for file in listdir("testdata/"):
        if file.endswith(".p4"):
            filename, extension = os.path.splitext(file)

            try:
                compile_p4_to_c(filename)
            except Exception as e:
                print(e)
                print("p4c - stopping compilation")
                break

            try:
                compile_c_to_o(filename)
                print("\n")
            except Exception as e:
                print(e)
                print("clang - stopping compilation")
                break


if __name__ == "__main__":
    main()

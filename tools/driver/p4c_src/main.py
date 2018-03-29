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

#!/usr/bin/env python

"""
p4c - P4 Compiler Driver

"""

from __future__ import absolute_import
import argparse
import glob
import os
import sys
import re

import p4c_src.config as config
import p4c_src

# \TODO: let the backends set their versions ...
p4c_version = p4c_src.__version__

def set_version(ver):
    global p4c_version
    p4c_version = ver

def get_version():
    return p4c_version


def display_supported_targets(cfg):
    ret = "Supported targets in \"target, arch\" tuple:\n"
    for target in cfg.target:
        ret += str(target) + "\n"
    return ret

def add_developer_options(parser):
    parser.add_argument("-T", dest="log_levels",
                        action="append", default=[],
                        help="[Compiler debugging] Adjust logging level per file (see below)")
    parser.add_argument("--top4", dest="passes",
                        action="append", default=[],
                        help="[Compiler debugging] Dump the P4 representation after \
                               passes whose name contains one of `passX' substrings. \
                               When '-v' is used this will include the compiler IR.")
    parser.add_argument("--dump", dest="dump_dir", default=None,
                        help="[Compiler debugging] Folder where P4 programs are dumped.")
    parser.add_argument("--toJson", dest="json", default=None,
                        help="Dump IR to JSON in the specified file.")
    parser.add_argument("--pp", dest="pretty_print", default=None,
                        help="Pretty-print the program in the specified file.")

def main():
    parser = argparse.ArgumentParser(conflict_handler='resolve')
    parser.add_argument("-V", "--version", dest="show_version",
                        help="show version and exit",
                        action="store_true", default=False)
    parser.add_argument("-v", "--debug", dest="debug",
                        help="verbose",
                        action="store_true", default=False)
    parser.add_argument("-###", "--test-only", dest="dry_run",
                        help="print (but do not run) the commands",
                        action="store_true", default=False)
    parser.add_argument("-Xpreprocessor", dest="preprocessor_options",
                        metavar="<arg>",
                        help="Pass <arg> to the preprocessor",
                        action="append", default=[])
    parser.add_argument("-Xp4c", dest="compiler_options",
                        metavar="<arg>",
                        help="Pass <arg> to the compiler",
                        action="append", default=[])
    parser.add_argument("-Xassembler", dest="assembler_options",
                        metavar="<arg>",
                        help="Pass <arg> to the assembler",
                        action="append", default=[])
    parser.add_argument("-Xlinker", dest="linker_options",
                        metavar="<arg>",
                        help="Pass <arg> to the linker",
                        action="append", default=[])
    parser.add_argument("-b", "--target", dest="target",
                        help="specify target device",
                        action="store", default="bmv2")
    parser.add_argument("-a", "--arch", dest="arch",
                        help="specify target architecture",
                        action="store", default="v1model")
    parser.add_argument("-c", dest="run_all",
                        help="Only run preprocess, compile, and assemble steps",
                        action="store_true", default=True)
    parser.add_argument("-D", dest="preprocessor_defines",
                        help="define a macro to be used by the preprocessor",
                        action="append", default=[])
    parser.add_argument("-E", dest="run_preprocessor_only",
                        help="Only run the preprocessor",
                        action="store_true", default=False)
    parser.add_argument("-e", dest="skip_preprocessor",
                        help="Skip the preprocessor",
                        action="store_true", default=False)
    parser.add_argument("-g", dest="debug_info",
                        help="Generate debug information",
                        action="store_true", default=False)
    parser.add_argument("-I", dest="search_path",
                        help="Add directory to include search path",
                        action="append", default=[])
    parser.add_argument("-o", dest="output_directory",
                        help="Write output to the provided path",
                        action="store", metavar="PATH", default=".")
    parser.add_argument("--p4runtime-file",
                        help="Write a P4Runtime control plane API description "
                        "to the specified file.",
                        action="store", default=None)
    parser.add_argument("--p4runtime-format",
                        choices=["binary", "json", "text"],
                        help="Choose output format for the P4Runtime API "
                        "description (default is binary).",
                        action="store", default="binary")
    parser.add_argument("--target-help", dest="show_target_help",
                        help="Display target specific command line options.",
                        action="store_true", default=False)
    parser.add_argument("-S", dest="run_till_assembler",
                        help="Only run the preprocess and compilation steps",
                        action="store_true", default=False)
    parser.add_argument("--std", "-x", dest="language",
                        choices = ["p4-14", "p4-16"],
                        help="Treat subsequent input files as having type language.",
                        action="store", default="p4-16")

    if (os.environ['P4C_BUILD_TYPE'] == "DEVELOPER"):
        add_developer_options(parser)

    parser.add_argument("source_file", nargs='?', help="Files to compile", default=None)

    # load supported configuration.
    # We load these before we parse options, so that backends can register
    # proprietary options
    cfg_files = glob.glob("{}/*.cfg".format(os.environ['P4C_CFG_PATH']))
    cfg = config.Config(config_prefix = "p4c")
    for cf in cfg_files:
        cfg.load_from_config(cf, parser)

    # parse the arguments
    opts = parser.parse_args()

    user_defined_version = os.environ.get('P4C_DEFAULT_VERSION')
    if user_defined_version != None:
        opts.language = user_defined_version

    user_defined_target = os.environ.get('P4C_DEFAULT_TARGET')
    if user_defined_target != None:
        opts.target = user_defined_target

    user_defined_arch = os.environ.get('P4C_DEFAULT_ARCH')
    if user_defined_arch != None:
        opts.arch = user_defined_arch

    # deal with early exits
    if opts.show_version:
        print "p4c", get_version()
        sys.exit(0)

    if opts.show_target_help:
        print display_supported_targets(cfg)
        sys.exit(0)

    if not opts.source_file:
        parser.error('No input specified.')

    # check that the tuple value is correct
    backend = (opts.target, opts.arch)
    if (len(backend) != 2):
        parser.error("Invalid target and arch tuple: {}\n{}".\
                     format(backend, display_supported_targets(cfg)))

    # find the backend
    backend = None
    for target in cfg.target:
        regex = target._backend.replace('*', '[a-zA-Z0-9*]*')
        pattern = re.compile(regex)
        if (pattern.match(opts.target + '-' + opts.arch)):
            backend = target
            break
    if backend == None:
        parser.error("Unknown backend: {}-{}".format(str(opts.target), str(opts.arch)))

    # set all configuration and command line options for backend
    backend.process_command_line_options(opts)
    # run all commands
    backend.run()

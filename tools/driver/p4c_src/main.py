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
import itertools
import os
import subprocess
import shlex
import sys
import re

import p4c_src.util as util
import p4c_src.config as config
import p4c_src

commands = {}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-V", "--version", dest="show_version",
                        help="show version and exit",
                        action="store_true", default=False)
    parser.add_argument("-v", dest="debug",
                        help="verbose",
                        action="store_true", default=False)
    parser.add_argument("-###", dest="dry_run",
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
    parser.add_argument("-b", dest="backend",
                        help="specify target backend",
                        action="store", default="bmv2-*-p4org")
    parser.add_argument("-E", dest="run_preprocessor_only",
                        help="Only run the preprocessor",
                        action="store_true", default=False)
    parser.add_argument("-e", dest="skip_preprocessor",
                        help="Skip the preprocessor",
                        action="store_true", default=False)
    parser.add_argument("-S", dest="run_till_assembler",
                        help="Only run the preprocess and compilation steps",
                        action="store_true", default=False)
    parser.add_argument("-c", dest="run_all",
                        help="Only run preprocess, compile, and assemble steps",
                        action="store_true", default=True)
    parser.add_argument("-x", dest="language",
                        choices = ["p4-14", "p4-16"],
                        help="Treat subsequent input files as having type language.",
                        action="store", default="p4-16")
    parser.add_argument("-I", dest="search_path",
                        help="Add directory to include search path",
                        action="append", default=[])
    parser.add_argument("-o", dest="output_directory",
                        help="Write output to the provided path",
                        action="store", metavar="PATH", default=".")
    parser.add_argument("--target-help", dest="show_target_help",
                        help="Display target specific command line options.",
                        action="store_true", default=False)

    parser.add_argument("source_file", nargs='?', help="Files to compile", default=None)

    # many more options
    opts = parser.parse_args()
    source = opts.source_file

    if opts.show_version:
        print("p4c %s" % (p4c_src.__version__))
        sys.exit(0)

    # target-arch-vendor, e.g.
    # bmv2-*-p4org
    # bmv2-ssa-p4org
    # ebpf-psa-p4org
    triplet = opts.backend.split('-')
    if (len(triplet) != 3):
        print "Invalid target-arch-vendor triplet."
        sys.exit(1)

    if not source:
        parser.error('No input specified.')

    # load supported configuration
    cfg_files = glob.glob("{}/*.cfg".format(os.environ['P4C_CFG_PATH']))
    cfg = config.Config(config_prefix = "p4c")
    for cf in cfg_files:
        if opts.debug:
            print 'loading config {}'.format(cf)
        cfg.load_from_config(cf, opts.output_directory, opts.source_file)

    if opts.show_target_help:
        print "Supported backends in \"target-arch-vendor\" triplet:"
        for target in cfg.target:
            print target
        sys.exit(0)

    backend = None
    for triplet, _ in cfg.steps.iteritems():
        regex = triplet.replace('*', '[a-zA-Z0-9*]*')
        pattern = re.compile(regex)
        if (pattern.match(opts.backend)):
            backend = triplet
            break
    if backend == None:
        print "Unknown backend:", opts.backend
        sys.exit(1)

    commands['preprocessor'] = []
    commands['compiler'] = []
    commands['assembler'] = []
    commands['linker'] = []

    for option in opts.preprocessor_options:
        commands["preprocessor"] += shlex.split(option)

    for option in opts.compiler_options:
        commands["compiler"] += shlex.split(option)

    for option in opts.assembler_options:
        commands["assembler"] += shlex.split(option)

    for option in opts.linker_options:
        commands["linker"] += shlex.split(option)

    # set output directory
    if not os.path.exists(opts.output_directory):
        os.makedirs(opts.output_directory)

    commands['preprocessor'].insert(0, cfg.get_preprocessor(backend))
    commands['compiler'].insert(0, cfg.get_compiler(backend))
    commands['assembler'].insert(0, cfg.get_assembler(backend))
    commands['linker'].insert(0, cfg.get_linker(backend))

    # handle mode flags
    step_enable = [False, False, False, False]
    if opts.run_preprocessor_only:
        step_enable = [True, False, False, False]
    elif opts.skip_preprocessor:
        step_enable = [False, True, True, True]
    elif opts.run_till_assembler:
        step_enable = [True, True, False, False]
    elif opts.run_all:
        step_enable = [True, True, True, True]

    # default search path
    if opts.language == 'p4-16':
        commands['preprocessor'].append("-I {}".format(os.environ['P4C_16_INCLUDE_PATH']))
        commands['compiler'].append("-I {}".format(os.environ['P4C_16_INCLUDE_PATH']))
    else:
        commands['preprocessor'].append("-I {}".format(os.environ['P4C_14_INCLUDE_PATH']))
        commands['compiler'].append("-I {}".format(os.environ['P4C_14_INCLUDE_PATH']))

    # append search path
    for path in opts.search_path:
        commands['preprocessor'].append("-I")
        commands['preprocessor'].append(path)
        commands['compiler'].append("-I")
        commands['compiler'].append(path)

    # set p4 version
    if opts.language == 'p4-16':
        commands['compiler'].append("--p4v=16")
    else:
        commands['compiler'].append("--p4v=14")

    for idx, step in enumerate(cfg.steps[backend]):
        cmd = []
        for c in commands[step]:
            cmd = cmd + shlex.split(c)
        options = cfg.options[backend][step]
        for option in options:
            cmd = cmd + shlex.split(option)
        # check if cmd in PATH
        if (util.find_bin(cmd[0]) == None):
            print "{}: command not found".format(cmd[0])
            sys.exit(1)
        # only dry-run
        if opts.dry_run:
            print "{}: {}".format(step, " ".join(cmd))
            continue
        # skip if not required
        if not step_enable[idx]:
            continue
        # run command
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if opts.debug:
            print 'running {}'.format(' '.join(cmd))
        out, err = p.communicate() # now wait
        if p.returncode != 0:
            print "{}\n{}".format(out, err)
            sys.exit(p.returncode)
        print out

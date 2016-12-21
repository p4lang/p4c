#!/usr/bin/env python
# Copyright 2016-present Barefoot Networks, Inc.
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
#
# This script examines the source tree, and automatically generates an automake
# file which can be included by Makefile.am to enable unified compilation. The
# idea of unified compilation is to create a "unified" C++ source file which
# simply #include's other source files.  Rather than compiling the original
# source files, we compile only the unified file, which greatly reduces
# compilation overhead in terms of I/O, duplicate template instantiations, and
# the like. This makes compiling the project much faster.
#
# Each automake file (i.e., file with '.am' extension) in the source tree is
# examined to find any variables of the form 'foo_UNIFIED' and 'foo_NONUNIFIED'.
# The script generates makefile rules that create a '.cpp' file that #include's
# the 'foo_UNIFIED' files. The '.cpp' file is added to 'foo_SOURCES' so that it
# will be compiled, and the usual automake features like dependency tracking
# work as expected. 'foo_NONUNIFIED' files are added directly to 'foo_SOURCES'
# and continue to be compiled separately.
#
# Putting every 'foo_UNIFIED' file in the same generated '.cpp' file would
# maximize single-threaded compilation speed, but it would limit parallelism and
# slow down incremental builds. This is why the 'foo_UNIFIED' files are split
# into chunks, with each chunk getting its own '.cpp' file which is compiled
# separately. The size of the chunks is a tunable parameter; if the default
# value isn't working well, it can be overridden by providing a custom chunk
# size as a parameter to this script.
#
# Correct usage requires setting up the makefiles appropriately. Since this
# script manages 'foo_SOURCES' itself, specifying 'foo_SOURCES' is an error if
# 'foo_UNIFIED' or 'foo_NONUNIFIED' is specified. This helps avoid unexpected
# interactions. 'foo_UNIFIED' also can't include any expansions ("$FOO" or
# "$(foo)") or any other makefile trickery, and it can't include file types
# other than '.c' and '.cpp' files. That includes headers; if you really want to
# include them somewhere, 'foo_NONUNIFIED' is the place, but the best thing is
# to leave them out altogether and let automake's automatic dependency
# management handle them.
#
# It's not necessary to worry about excluding automake files that aren't
# relevant to the target you're building. This script only generates make rules;
# if you're not building a target that triggers them, they won't have any
# effect.
#
# Basic usage of this script is to run it from the root of the source tree
# before automake is run, passing '-o unified-compilation.am' to specify the
# output file. The output file then needs to be included from your Makefile.am.
# The script requires that Makefile.am also set a GEN_UNIFIED_CPP
# variable which points to the script that will actually create the generated
# '.cpp' files at build time. (That's `gen-unified-sources.py`.) Doing it this way
# allows this tool to run before the build directory has been created, and
# avoids generating '.cpp' files that aren't needed by the targets we're
# building.
#
# If you want to support automatically regenerating the unified compilation
# information when one of the input automake files changes, pass
# '--regenerate-with regenerate-unified-compilation.am' when you run this script
# and include the resulting file in your Makefile.am. You'll also need to set a
# GEN_UNIFIED_MAKEFILE variable which points to this script.
#
# Information about other command line arguments is available by passing
# '--help' to the script.

from __future__ import print_function
import argparse
import collections
import fnmatch
import math
import os
import re
import stat
import sys

def find_automake_files(excluded_files):
    files = []
    for path_prefix, _, filenames in os.walk('.', followlinks=True):
        for filename in fnmatch.filter(filenames, '*.am'):
            path = os.path.join(path_prefix, filename)
            relative_path = os.path.relpath(path, '.')
            if relative_path not in excluded_files:
                files.append(path)

    return files

def construct_file_set(makefile, reldir, target, type, files):
    for file in files:
        # Filter empty filenames. These end up getting passed to us because of
        # split()'s behavior when the regular expression contains a capture
        # group.
        if file == '':
            continue

        # Replace %reldir% replaced with a concrete path.
        file = file.replace('%reldir%', reldir)

        # If this is a '_UNIFIED' file, we'll ultimately need to generate a C++
        # source file that includes it, so it needs additional validation. We
        # can be more permissive for other files since they are simply passed
        # through to the underlying makefile.
        if type == '_UNIFIED':
            # We can't handle expansions ('$FOO or $(foo)') in 'foo_UNIFIED',
            # because we're not actually evaluating the makefile, so report an
            # error if we find any.
            if '$' in file:
                raise Exception('{}: {}{} includes file "{}" containing an expansion, '
                                'which is not supported.'.format(makefile, target, type, file))

            # Verify that the file type is acceptable for unified compilation.
            # Note that although we *could* allow header files, they're not
            # useful, and we'd have to filter them out anyway or they'd
            # interfere with accurate dependency tracking.
            (_, extension) = os.path.splitext(file)
            if extension not in ['.c', '.cpp']:
                raise Exception('{}: {}{} includes file "{}" which is not a .c or .cpp '
                                'source file.'.format(makefile, target, type, file))

        yield file

def compute_file_sets(automake_files):
    # This regular expression matches expressions that set 'foo_UNIFIED' or
    # 'foo_NONUNIFIED' variables in a makefile. Quotes and escaping are not
    # supported in filenames, so don't get too crazy. =) 'foo_SOURCES' is also
    # detected for error reporting; see below.
    unified_re = re.compile(
        r"^(?P<target>\w+?)"                       # The target name ('foo').
        r"(?P<type>_UNIFIED|_NONUNIFIED|_SOURCES)" # The type of file set.
        r" *(?:\+|:|::|\?)?\="                     # Some sort of assignment or append.
        r"(?P<files>(( |\t|\\\n)*[^ \t\\\n]+)*)$", # A possibly-multiline sequence of files.
        re.MULTILINE)

    # This regular expression is used for splitting the sequence of files matched by
    # the previous regular expression into a list of individual filenames.
    file_split_re = re.compile(r"(?: |\t|\\\n)+")

    # Collect all of the 'foo_UNIFIED' and 'foo_NONUNIFIED' assignments for each
    # 'foo'. There's no attempt to handle makefile semantics; regardless of the
    # type of assignment or append, we treat everything as if it were a '+='.
    unified_file_sets = collections.defaultdict(set)
    nonunified_file_sets = collections.defaultdict(set)
    targets_with_sources_vars = set()
    for makefile_name in automake_files:
        # Determine the value of %reldir% in this makefile. Automake sets it to
        # the path relative to the top-level Makefile.am. Rather than attempt to
        # implement those semantics precisely, we simply use the path relative
        # to the current directory.
        reldir = os.path.relpath(os.path.dirname(makefile_name), '.')

        with open(makefile_name, 'r') as makefile:
            for assignment in unified_re.finditer(makefile.read()):
                (target, type, files) = assignment.group('target', 'type', 'files')
                file_set = construct_file_set(makefile, reldir, target, type,
                                              file_split_re.split(files))

                if type == '_UNIFIED':
                    unified_file_sets[target].update(file_set)
                elif type == '_NONUNIFIED':
                    nonunified_file_sets[target].update(file_set)
                elif type == '_SOURCES':
                    targets_with_sources_vars.add(target)
                else:
                    raise Exception('Unexpected type {}'.format(type))

    # We report an error if there's a 'foo_SOURCES' assignment for a target that
    # has 'foo_UNIFIED' or 'foo_NONUNIFIED' assignments; mixing the two styles
    # is asking for trouble.
    unified_targets = set(unified_file_sets.keys()).union(nonunified_file_sets.keys())
    targets_with_invalid_sources = targets_with_sources_vars.intersection(unified_targets)
    if targets_with_invalid_sources:
        raise Exception('*_SOURCES is set for targets which define *_UNIFIED or '
                        '*_NONUNIFIED: {}'.format(targets_with_invalid_sources))

    # We report an error if the same file appears in 'foo_UNIFIED' and
    # 'foo_NONUNIFIED'; this would result in the file being built twice, and
    # almost certainly in errors at link time.
    for target in unified_targets:
        duplicated_files = unified_file_sets[target].intersection(nonunified_file_sets[target])
        if duplicated_files:
            raise Exception('The same files are present in both {}_UNIFIED and '
                            '{}_NONUNIFIED: {}'.format(target, duplicated_files))

    return (unified_targets, unified_file_sets, nonunified_file_sets)

def generate_preamble(output, script_name, max_chunk_size):
    print('###################################################################', file=output)
    print('# Unified compilation automake file.', file=output)
    print('# Generated by:', script_name, file=output)
    print('# Maximum chunk size:', max_chunk_size, file=output)
    print('# DO NOT EDIT. Changes will be overwritten.', file=output)
    print('###################################################################', file=output)
    print(file=output)

def chunk(max_chunk_size, file_set):
    """ Split `file_set` into chunks with at most `max_chunk_size` elements. """
    if len(file_set) == 0:
        return []

    needed_chunks = math.ceil(len(file_set) / float(max_chunk_size))
    even_chunk_size = int(math.ceil(len(file_set) / needed_chunks))

    file_list = list(file_set)
    return [file_list[i:i + even_chunk_size]
            for i in xrange(0, len(file_list), even_chunk_size)]

def generate_rules(output, target, unified_file_chunks, nonunified_file_set):
    # Non-unified files are just passed through directly to 'foo_SOURCES'.
    nonunified_files = ' '.join(nonunified_file_set)
    print('{}_SOURCES = {}'.format(target, nonunified_files), file=output)
    print(file=output)

    # For each chunk of unified files, we generate a rule that creates a unified
    # .cpp file which #include's all those files, and then add the unified .cpp
    # file to 'foo_SOURCES'. The generation of the unified .cpp file is actually
    # performed at build time using a tool specified in Makefile.am by setting
    # GEN_UNIFIED_CPP. This allows this script to be run before the build
    # directory is created.
    for index in xrange(0, len(unified_file_chunks)):
        files_in_chunk = ' '.join(unified_file_chunks[index])
        chunk_target = 'unified-sources-{}-{}.cpp'.format(target, index)

        print('{}: $(GEN_UNIFIED_CPP) {} {}'
                .format(chunk_target, output.name, files_in_chunk), file=output)
        print('\t@$(GEN_UNIFIED_CPP) {} > $@'.format(files_in_chunk), file=output)
        print(file=output)
        print('{}_SOURCES += {}'.format(target, chunk_target), file=output)
        print(file=output)

def generate_regen_file(regen, args, automake_files):
    # Generate an automake file which will rerun this script with the same
    # arguments when the input automake files change. We can't put these rules
    # in the main output automake file because it would create a circular
    # dependency between the rules in the file and the file itself; make
    # identifies and ignores such dependencies.
    print('###################################################################', file=regen)
    print('# Utility makefile to regenerate unified compilation information.', file=regen)
    print('# Generated by:', sys.argv[0], file=regen)
    print('# DO NOT EDIT. Changes will be overwritten.', file=regen)
    print('###################################################################', file=regen)
    print(file=regen)

    # The dependencies need to be $(srcdir) relative. The script should've either
    # been run from $(srcdir), or the user should've passed '--root $(srcdir)'
    # at the command line, so we can just rewrite the paths relative to '.'.
    target = '$(srcdir)/' + os.path.relpath(args.output, '.')
    dependencies = ' '.join(['$(srcdir)/' + os.path.relpath(f, '.') for f in automake_files])

    print('{}: $(GEN_UNIFIED_MAKEFILE) {}'.format(target, dependencies), file=regen)
    print('\t@$(GEN_UNIFIED_MAKEFILE) --max-chunk-size {} --root $(srcdir) --regenerate-with {} -o {}'
            .format(args.max_chunk_size, args.regen, args.output), file=regen)
    print(file=regen)

def main(argv):
    parser = argparse.ArgumentParser(description='Search for automake files and '
                                                 'generate a unified compilation '
                                                 'automake file from them. See the '
                                                 'source for more information.')
    parser.add_argument('--max-chunk-size', nargs='?', default=10,
                        type=int, metavar='N',
                        help='The maximum number of files included by each of the '
                             'generated .cpp files. Increase to make full rebuilds '
                             'faster; decrease to make incremental rebuilds faster. '
                             'Defaults to 10.')
    parser.add_argument('--root', nargs='?', default='.',
                        type=str, metavar='DIR',
                        help='Search for input automake files in subdirectories '
                             'of DIR. Automake files in DIR itself are ignored. '
                             'This should be the same as $(srcdir) in your '
                             'top-level Makefile.am. The output files are also '
                             'specified relative to this directory. Defaults to '
                             'the current directory.')
    parser.add_argument('--regenerate-with', nargs='?', dest='regen',
                        type=str, metavar='REGEN.am',
                        help='Write rules into REGEN.am to rerun this tool when '
                             'the input automake files change.')
    parser.add_argument('-o', required=True, dest='output',
                        type=str, metavar='OUTPUT.am',
                        help='Write the unified compilation automake file into '
                             'OUTPUT.am.')
    args = parser.parse_args()

    # Change to the source tree root and open the output files. The order is
    # important since the output files are specified relative to the root.
    os.chdir(args.root)
    output = open(args.output, 'w')
    regen = open(args.regen, 'w') if args.regen else None

    # Start by locating all automake files in the source tree. We don't need to
    # do anything clever about e.g. excluding automake files that we don't
    # intend on actually including in the makefile, because we're just
    # generating rules at this point. The rules won't actually be invoked unless
    # unless there's some target that depends on them. (We do have to exclude
    # the files we're generating, though, or else we'll confuse ourselves.)
    excluded_files = [args.output, args.regen]
    automake_files = find_automake_files(excluded_files)

    # Now we compute the file sets. This is designed to fit right in with
    # automake's approach. We look for make variables named 'foo_UNIFIED' (the
    # files which should be compiled in unified mode) and 'foo_NONUNIFIED' (the
    # files which should be compiled separately); the makefile we generate will
    # set 'foo_SOURCES' with appropriate dependencies, and automake/make will do
    # the rest.
    (unified_targets, unified_file_sets, nonunified_file_sets) = compute_file_sets(automake_files)

    # We now have enough information to generate the automake file.
    generate_preamble(output, argv[0], args.max_chunk_size)
    for target in unified_targets:
        unified_file_chunks = chunk(args.max_chunk_size, unified_file_sets[target])
        generate_rules(output, target, unified_file_chunks, nonunified_file_sets[target])

    # If requested, generate rules to rerun this tool when the inputs change.
    if regen is not None:
        generate_regen_file(regen, args, automake_files)

if __name__ == "__main__":
    main(sys.argv)

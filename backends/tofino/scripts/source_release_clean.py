#!/usr/bin/env python3

#
# Clean the source code prior to open-sourcing/publicly releasing
#

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# Top-level source dirs
SRC_DIRS = ['bf-asm', 'bf-p4c']

parser = argparse.ArgumentParser(description='Clean the source prior to public release')
parser.add_argument('-r', '--root', help='Root directory of the compile source tree')
args = parser.parse_args()

# This script must either be invoked from the top-level directory or with a
# root parameter
if args.root:
    os.chdir(args.root)

# Check for the presence of bf-asm and bf-p4c directorys
subdirs_present = all([os.path.exists(x) for x in SRC_DIRS])

if not subdirs_present:
    print(
        "Error: '{}' does not appear to be the source tree top-level "
        "directory. Expecting to find subdirectories: "
        "{}.".format(os.getcwd(), SRC_DIRS),
        file=sys.stderr,
    )
    sys.exit(1)

# Set up paths
CPPP = os.getcwd() + '/scripts/_deps/cppp/cppp'


# =======================================================
# Verify that deps are present
# =======================================================

PREREQ_SCRIPT = os.getcwd() + '/scripts/install_source_release_clean_prereqs.sh'

if not os.path.exists(CPPP):
    print("Error: cannot find {}".format(CPPP), file=sys.stderr)
    print("".format(CPPP), file=sys.stderr)
    print("Have you run {}?".format(PREREQ_SCRIPT), file=sys.stderr)
    sys.exit(1)


# =======================================================
# Remove source code corresponding to unreleased hardware
# =======================================================

UNRELEASED_MACROS = ['HAVE_CLOUDBREAK', 'HAVE_FLATROCK']

CPPP_CMD_BASE = [CPPP]
CPPP_CMD_BASE[1:] = ['-U' + x for x in UNRELEASED_MACROS]


def run_cppp(src_dir):
    '''
    Run cppp (c partial preprocessor) on all files in a directory

    Recursively call run_cppp on all subdirectories
    '''
    print("Running cppp on:", src_dir)
    for file in os.listdir(src_dir):
        path = os.path.join(src_dir, file)
        if os.path.isdir(path):
            run_cppp(path)
        elif file.endswith(".c") or file.endswith(".cpp") or file.endswith(".h"):
            tmp_path = path + ".new"
            CPPP_CMD = CPPP_CMD_BASE + [path, tmp_path]
            subprocess.run(CPPP_CMD)
            shutil.move(tmp_path, path)


def sed_replace(path, src, dst, include_cmake=False):
    '''
    Perform a replace with sed on all .cpp and .h files, and optionally cmake
    files, within path. The src and dst strings are specified as parameters.
    '''
    print("Replacing '{}' with '{}' in {}".format(src, dst, path))
    cmd = "find " + path + " -name '*.cpp' -or -name '*.h' -or -name '*.def'"
    cmd += " -or -name '*.py' -or -name 'SYNTAX.yaml'"
    if include_cmake:
        cmd += " -or -name CMakeLists.txt"
    cmd += " | xargs sed -i 's/" + src + "/" + dst + "/g'"
    subprocess.run([cmd], shell=True)


SRC_ONLY = ['*.cpp', '*.h', '*.def', '*.py', 'SYNTAX.yaml']
CMAKE_LISTS = ['CMakeLists.txt']
CMAKE_ALL = ['CMakeLists.txt', '*.cmake']

SRC_AND_CMAKE = SRC_ONLY + CMAKE_LISTS


def sed_delete(path, del_exp, patterns=SRC_ONLY):
    '''
    Delete with sed on all .cpp and .h files, and optionally cmake files,
    within path. The delete expression is specified as a parameter.
    '''
    print("Deleting '{}' in {}".format(del_exp, path))
    cmd = "find " + path
    sep = ""
    for pattern in patterns:
        cmd += sep + " -name '{}'".format(pattern)
        sep = " -or"
    cmd += " | xargs sed -i '/" + del_exp + "/d'"
    subprocess.run([cmd], shell=True)


for src_dir in SRC_DIRS:
    run_cppp(src_dir)

    # Strip out any remaining references to the macros
    #
    # These occur when the macro is references in a comment, typically
    # in #endif /* HAVE_XYZ */
    sed_re = r'\|'.join(UNRELEASED_MACROS)
    sed_replace(src_dir, sed_re, '')


# =======================================================
# Remove directories/files for unreleased targets
# =======================================================

UNRELEASED_TARGETS = ['cloudbreak', 'flatrock', 'tofino3', 't5na', 't5na_program_structure']

for target in UNRELEASED_TARGETS:
    for path in sorted(Path().rglob(target)):
        print("Deleting tree:", path.as_posix())
        shutil.rmtree(path.as_posix())
    for path in sorted(Path().rglob(target + '.*')):
        print("Deleting file:", path.as_posix())
        path.unlink()

EXTRA_DELETES = [
    'bf-p4c/parde/lowered/lower_flatrock.h',
    'bf-p4c/p4include/t3na.p4',
    'bf-p4c/p4include/tofino3_arch.p4',
    'bf-p4c/p4include/tofino3_base.p4',
    'bf-p4c/p4include/tofino3_specs.p4',
    'bf-p4c/p4include/tofino5arch.p4',
    'bf-p4c/p4include/tofino5.p4',
    'bf-p4c/driver/p4c.tofino3.cfg',
    'bf-p4c/driver/p4c.tofino5.cfg',
]
for path in EXTRA_DELETES:
    if os.path.exists(path):
        print('Deleting', path)
        os.unlink(path)

# =======================================================
# Rename files
# =======================================================

# Files to rename
RENAMES = {
    "bf-asm/parser-tofino-jbay-cloudbreak.h": "bf-asm/parser-tofino-jbay.h",
    "bf-asm/parser-tofino-jbay-cloudbreak.cpp": "bf-asm/parser-tofino-jbay.cpp",
}

# Per-directory src string substitutions
SRC_SUBSTITUES = {
    "bf-asm": {
        "parser-tofino-jbay-cloudbreak.h": "parser-tofino-jbay.h",
        "parser-tofino-jbay-cloudbreak.cpp": "parser-tofino-jbay.cpp",
        "PARSER_TOFINO_JBAY_CLOUDBREAK_H_": "PARSER_TOFINO_JBAY_H_",
    },
    "bf-p4c": {
        r"\s*\/\/ \(Tofino5\|Flatrock\) specific$": "",
        r"\s*\/\/ TODO for \(Tofino5\|[Ff]latrock\)$": "",
    },
}

for src, dst in RENAMES.items():
    if os.path.exists(src):
        print('Renaming:', src, '->', dst)
        os.rename(src, dst)

for path, subs in SRC_SUBSTITUES.items():
    for src, dst in subs.items():
        sed_replace(path, src, dst, True)


# =======================================================
# Delete documentation strings
# =======================================================

# Documentation strings to delete
DOC_STRINGS = ['.*\<\(JIRA\|TOF[35]\)-DOC:.*']

for path in SRC_DIRS:
    for del_str in DOC_STRINGS:
        sed_delete(path, del_str, SRC_AND_CMAKE)

for del_str in DOC_STRINGS:
    sed_delete('p4-tests', del_str, CMAKE_ALL)


# =======================================================
# Clean p4 include headers
# =======================================================

P4_INCLUDE_FILTER = ['CLOUDBREAK_ONLY', 'FLATROCK_ONLY']
P4_INCLUDE_DIRS = ['bf-p4c/p4include']

p4_include_re = r'/\<\(' + '\|'.join(P4_INCLUDE_FILTER) + r'\)\>/d'

for path in P4_INCLUDE_DIRS:
    for inc in glob.glob(path + '/*.p4'):
        subprocess.run(['sed', '-i', p4_include_re, inc])

# =======================================================
# cmake updates
#  - difficult to precisely target correct lines
# =======================================================

print("Cleaning cmake files")
targets = UNRELEASED_TARGETS + ['lower_flatrock', 't5na', 't5na_program_structure']
for src_dir in SRC_DIRS:
    for path in sorted(Path(src_dir).rglob('CMakeLists.txt')):
        print('Processing', path.as_posix())
        for target in targets:
            sed_re = r's/.*[ /]' + target + r'\.\(cpp\|h\|def\).*//'
            subprocess.run(['sed', '-i', sed_re, path.as_posix()])


# Read the CMakeLists.txt files and filter out various enables
FILTER_CMAKE = {'ENABLE_CLOUDBREAK', 'ENABLE_FLATROCK', 'CLOSED_SOURCE'}
IF_RE = re.compile(r'^\s*if\s*\((.*)\)')
ELSEIF_RE = re.compile(r'^\s*elseif\s*\((.*)\)')
ELSE_RE = re.compile(r'^\s*else\s*\((.*)\)')
ENDIF_RE = re.compile(r'^\s*endif\s*\((.*)\)')
NOT_RE = re.compile(r'(?i)\bNOT\s+(\w+)')
SET_RE = re.compile(r'^\s*set\s*\(\s*(\w+)')


class IfContext:
    '''
    If statement context tracking for use in cmake clean-up
    '''

    def __init__(self, filtering, if_emitted, skip_rest):
        # Params:
        #  - filtering  = currently in an element being filtered
        #  - if_emitted = emitted an if / need the endif
        #  - skip_rest  = skip remaining branches
        self.filtering = filtering
        self.if_emitted = if_emitted
        self.skip_rest = skip_rest


def clean_cmake_ifs(path):
    '''
    Clean up cmake files by removing if statement branches

    '''
    dst_fp = tempfile.NamedTemporaryFile(mode='w', delete=False)
    with open(path) as src:
        # If statement context:
        # List:
        #  - Last element is the current (inner-most) if statement
        #  - Element 0 is the outer-most if statement
        #  - Empty list means that we are not currently in an if staement
        #
        # List elements: IfContext objects
        if_stack = []
        line_no = 1
        changed = False
        for line in src:
            line_adj = line.rstrip()

            # Suboptimal to blindly run all 4 regular expressions
            # But this script is not used frequently and doesn't need to be
            # highly optimized
            m_if = IF_RE.match(line_adj)
            m_elseif = ELSEIF_RE.match(line_adj)
            m_else = ELSE_RE.match(line_adj)
            m_endif = ENDIF_RE.match(line_adj)
            m_set = SET_RE.match(line_adj)

            # DEBUG:
            # if m_if or m_elseif or m_else or m_endif:
            #     print('--', line_adj)

            # Verify that we have a context for elseif/else/endif
            if m_elseif or m_else or m_endif:
                if len(if_stack) == 0:
                    if m_elseif:
                        keyword = 'elseif'
                    elif m_else:
                        keyword = 'else'
                    else:
                        keyword = 'endif'
                    print('ERROR: {} without an if in {}:{}'.format(keyword, path, line_no))
                    print('Skipping processing of file')
                    return

            exp = ""
            inv = False
            if m_if or m_elseif:
                if m_if:
                    exp = m_if.group(1)
                else:
                    exp = m_elseif.group(1)

                m_not = NOT_RE.match(exp)
                if m_not:
                    inv = True
                    exp = m_not.group(1)
            exp = exp.strip()

            if m_if:
                if len(if_stack) > 0 and if_stack[-1].filtering:
                    if_stack.append(IfContext(True, False, True))
                    changed = True
                else:
                    if exp in FILTER_CMAKE:
                        # Starting an if statement with one of the filter terms
                        if inv:
                            # NOT filter term: skip the if statement but keep the block
                            # End after this as this condition is true
                            if_stack.append(IfContext(False, False, True))
                        else:
                            # filter term: skip the if statement and block
                            if_stack.append(IfContext(True, False, False))
                            changed = True
                    else:
                        # Starting an if with something _other_ than a filter term
                        # Keep the if statement
                        if_stack.append(IfContext(False, True, False))
                        dst_fp.write(line)
            elif m_elseif:
                if if_stack[-1].skip_rest:
                    if_stack[-1].filtering = True
                    changed = True
                else:
                    if exp in FILTER_CMAKE:
                        # Starting an if statement with one of the filter terms
                        if inv:
                            # NOT filter term: skip the if statement but keep the block
                            # End after this as this condition is true
                            if if_stack[-1].if_emitted:
                                # Already emitted an if -> convert to else
                                dst_fp.write('else')
                            if_stack[-1].filtering = False
                            if_stack[-1].skip_rest = True
                        else:
                            # filter term: skip the if statement and block
                            if_stack[-1].filtering = True
                            changed = True
                    else:
                        # Continuing an if with something _other_ than a filter term
                        # Keep the if statement
                        if not if_stack[-1].if_emitted:
                            # Haven't emited an if -> convert to if
                            line = re.sub(r'\belseif\b', 'if', line)
                            if_stack[-1].if_emitted = True
                        if_stack[-1].filtering = False
                        dst_fp.write(line)
            elif m_else:
                if if_stack[-1].skip_rest:
                    if_stack[-1].filtering = True
                    changed = True
                else:
                    if if_stack[-1].if_emitted:
                        dst_fp.write(line)
                    if_stack[-1].filtering = False
            elif m_endif:
                if if_stack[-1].if_emitted:
                    dst_fp.write(line)
                if_stack.pop()
            else:
                if len(if_stack) == 0 or not if_stack[-1].filtering:
                    if m_set and m_set.group(1).strip() in FILTER_CMAKE:
                        pass
                    else:
                        dst_fp.write(line)

            line_no += 1

        dst_fp.close()
        if changed:
            shutil.move(dst_fp.name, path)
        else:
            os.unlink(dst_fp.name)


EXTRA_CMAKE_FILES = ['CMakeLists.txt']
for path in EXTRA_CMAKE_FILES:
    print('Filtering content of', path)
    clean_cmake_ifs(path)

for src_dir in SRC_DIRS:
    for path in sorted(Path(src_dir).rglob('CMakeLists.txt')):
        print('Filtering content of', path.as_posix())
        clean_cmake_ifs(str(path))

for src_dir in ['p4-tests']:
    for path in sorted(Path(src_dir).glob('*.cmake')):
        print('Filtering content of', path.as_posix())
        clean_cmake_ifs(str(path))

# =======================================================
# Miscellaneous clean steps
# =======================================================

SED_EXTRA = {
    'bf-asm/cmake/config.h.in': '/flatrock\|cloudbreak/Id',
    'bf-asm/test/runtests': [
        '/STF3/d',
        '/tofino3/d',
    ],
    'bf-asm/instruction.cpp': [
        's/tofino123/tofino12/g',
        's/jb_cb_/jb_/g',
    ],
}

for path, cmds in SED_EXTRA.items():
    if type(cmds) is not list:
        cmds = [cmds]
    for cmd in cmds:
        print("Running sed '{}' on {}".format(cmd, path))
        subprocess.run(['sed', '-i', cmd, path])

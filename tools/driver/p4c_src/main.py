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


"""
p4c - P4 Compiler Driver

"""


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


JSON_input_flag = "--fromJson"  ### DRY
def add_developer_options(parser):
    parser.add_argument("-T", dest="log_levels",
                        action="append", default=[],
                        help="[Compiler debugging] Adjust logging level"
                        " per file (see below)")
    parser.add_argument("--top4", dest="passes",
                        action="append", default=[],
                        help="[Compiler debugging] Dump the P4 representation"
                             " after passes whose name contains one of 'passX'"
                             " substrings.  When '-v' is used this will"
                             " include the compiler IR.")
    parser.add_argument("--dump", dest="dump_dir", default=None,
                        help="[Compiler debugging] Folder where P4 programs"
                             " are dumped.")
    parser.add_argument("--toJson", dest="json", default=None,
                        help="Dump IR to JSON in the specified file.")
    parser.add_argument(JSON_input_flag, dest="json_source", default=None,
                        help="Use IR from JSON representation dumped"
                             " previously.  The compilation starts with a"
                             " reduced midEnd.  Only allowed/recognized in"
                             " developer builds.")
    parser.add_argument("--pp", dest="pretty_print", default=None,
                        help="Pretty-print the program in the specified file.")


### a utility function for grammatical correctness [and DRY]
def s_and_were_or_just_was(parameter):
    return ("s were" if parameter != 1 else " was")


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
    parser.add_argument("-c", "--compile", dest="run_all",
                        help="Only run the preprocess, compile, and assemble"
                        " steps", action="store_true", default=True)
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
    parser.add_argument("-o", "--output", dest="output_directory",
                        help="Write output to the provided path",
                        action="store", metavar="PATH", default=".")
    parser.add_argument("--p4runtime-file",
                        help="Write a P4Runtime control plane API description "
                        "to the specified file. "
                        "[Deprecated; use '--p4runtime-files' instead]",
                        action="store", default=None)
    parser.add_argument("--p4runtime-files",
                        help="Write the P4Runtime control plane API "
                        "description (P4Info) to the specified files "
                        "(comma-separated list); the format is detected based"
                        " on the filename suffix. "
                        ' Legal suffixes are ".txt", ".json", and ".bin".',
                        action="store", default=None)
    parser.add_argument("--p4runtime-format",
                        choices=["binary", "json", "text"],
                        help="Choose output format for the P4Runtime API "
                        "description (default is binary). "
                        "[Deprecated; use '--p4runtime-files' instead]",
                        action="store", default="binary")
    parser.add_argument("--help-pragmas", "--pragma-help", "--pragmas-help",
                        "--help-annotations", "--annotation-help",
                        "--annotations-help", dest="help_pragmas",
                        action="store_true", default=False,
                        help="Print the documentation about supported"
                             " annotations/pragmas and exit.")
    parser.add_argument("--help-targets", "--target-help", "--targets-help",
                        dest="show_target_help",
                        help="Display target specific command line options.",
                        action="store_true", default=False)
    parser.add_argument("--disable-annotations", "--disable-annotation",
                        "--disable-pragmas", "--disable-pragma",
                        dest="disabled_annos", action="store",
                        help="List of (comma-separated) annotations that"
                             " should be ignored by the compiler.")
    parser.add_argument("-S", dest="run_till_assembler",
                        help="Only run the preprocess and compilation steps",
                        action="store_true", default=False)
    parser.add_argument("--std", "-x", dest="language",
                        choices=["p4-14", "p4_14", "p4-16", "p4_16"],
                        help="Treat subsequent input files as having contents "
                             "of the type indicated by "
                             "the parameter to this argument.",
                        action="store", default="p4-16")
    parser.add_argument("--ndebug", dest="ndebug_mode",
                        help="Compile program in non-debug mode.\n",
                        action="store_true", default=False)
    parser.add_argument("--parser-inline-opt", dest="optimizeParserInlining",
                        help="Enable optimization of inlining of callee "
                             "parsers (subparsers). "
                             "The optimization is disabled by default. "
                             "When the optimization is disabled, "
                             "for each invocation of the subparser "
                             "all states of the subparser are inlined, "
                             "which means that the subparser "
                             "might be inlined multiple times even if "
                             "it is the same instance "
                             "which is invoked multiple times. "
                             "When the optimization is enabled, the compiler "
                             "tries to identify the cases where it can inline"
                             " the subparser's states only once for multiple "
                             "invocations of the same subparser instance.",
                        action="store_true", default=False)

    ### DRYified “env_indicates_developer_build”
    env_indicates_developer_build = os.environ['P4C_BUILD_TYPE'] == "DEVELOPER"
    if env_indicates_developer_build:
        add_developer_options(parser)

    parser.add_argument("P4_source_files", nargs='*',
                        help="P4 source files to compile.", default=None)

    # load supported configuration.
    # We load these before we parse options, so that backends can register
    # proprietary options
    cfg_files = glob.glob("{}/*.cfg".format(os.environ['P4C_CFG_PATH']))
    cfg = config.Config(config_prefix="p4c")
    for cf in cfg_files:
        cfg.load_from_config(cf, parser)
    ### ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ###
    ### Abe`s note: I hope my {“source_file” → “P4_source_files”} change
    ###   (to the “parser” object) at this point in time can`t cause any
    ###   ill effects in the callee “config.Config.load_from_config”.
    ###
    ###   WIP To Do: check with somebody more experienced with this codebase
    ###              before even _thinking_ about merging this changeset into “main”.

    # parse the arguments
    opts = parser.parse_args()

    user_defined_version = os.environ.get('P4C_DEFAULT_VERSION')
    if user_defined_version != None:
        opts.language = user_defined_version
    # Accept multiple ways of specifying which language,
    #   and ensure that it is a consistent string from now on.
    if  opts.language == "p4_14":
        opts.language =  "p4-14"
    if  opts.language == "p4_16":
        opts.language =  "p4-16"

    user_defined_target = os.environ.get('P4C_DEFAULT_TARGET')
    if user_defined_target != None:
        opts.target = user_defined_target

    user_defined_arch = os.environ.get('P4C_DEFAULT_ARCH')
    if user_defined_arch != None:
        opts.arch = user_defined_arch

    # deal with early exits
    if opts.show_version:
        print("p4c", get_version())
        sys.exit(0)

    if opts.show_target_help:
        print(display_supported_targets(cfg))
        sys.exit(0)

    # check that the tuple value is correct
    backend = (opts.target, opts.arch)
    if (len(backend) != 2):
        parser.error("Invalid target and arch tuple: {}\n{}".
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
        parser.error("Unknown backend: {}-{}".format(str(opts.target),
                                                     str(opts.arch)))
    error_count = 0

    JSON_input_specified = env_indicates_developer_build and opts.json_source
### ^^^^^^^^^^^^^^^^^^^^: DRY
    P4_input_or_inputs_specified = len(opts.P4_source_files) > 0
    any_input_specified = JSON_input_specified or P4_input_or_inputs_specified
### ^^^^^^^^^^^^^^^^^^^: for readability

    if JSON_input_specified and P4_input_or_inputs_specified:
        ### Note: this restriction was added by Abe, so if there is a test case which
        ###       tries to feed the driver both a JSON input pathname via the relevant
        ###       flag _and_ another input pathname, that will now fail whereas before the
        ###       JSON input pathname would silently override the other input pathname[s].
        print("\nERROR: it seems that both (firstly) a JSON input pathname "
                'was specified via "' + JSON_input_flag + '"' +
                " _and_ (secondly) " + str(len(opts.P4_source_files)) +
                " other input pathname" +
                s_and_were_or_just_was(len(opts.P4_source_files)) +
                " specified.  A JSON input at the same time as a P4 input is"
                " currently unsupported.")
        error_count = 1
        ### Replace ↑ ‘=’ with “+=” if/when ever moving this line of code
        ###   to somewhere it might not be the first error detection.

    # When using --help-* options, we don't necessarily need to pass an input file
    # However, by default the driver checks the input and fails if it does not exist.
    # In that case we set source to dummy.p4 so sanity checking works. Backend can
    # force this behaviour for its own help options by overriding the
    # should_not_check_input method.
    checkInput = not (opts.help_pragmas or backend.should_not_check_input(opts))

    use_a_dummy_P4_pseudoPathname = False
    ### a note from Abe: do we really still need the dummy pathname?
    ###                  Maybe we can take all the dummy-related code out once my work to improve the
    ###                    pathname handling in the driver is good enough for merging to “main”.
    if checkInput:
        if not any_input_specified:
            parser.error("no input pathname was specified.")
            ### reminder: at this point, the program is dead

        if len(opts.P4_source_files) > 1:
            print("\nSorry; as of this writing, the P4 compiler driver does "
                    "_not_ support multiple top-level P4 source files in a " # noqa
                    "single invocation.  Multiple P4 source files at a time "
                    'are currently only supported via "#include" (i.e. '
                    "additional non-top-level P4 source files).  "
                    "Number of top-level P4 source-file pathnames detected: " +
                    str(len(opts.P4_source_files)), file=sys.stderr)
            error_count += 1

        ### We need to do the next 3 lines of code this way (instead of
        ###   iterating over a concise “list if list else []” expression)
        ###   because “opts.json_source” might be invalid [i.e. we are _not_ in a developer build].
        pathnames_to_check = opts.P4_source_files.copy()
        ### Abe`s reminder to self re the preceding line: a plain “pathnames_to_check = opts.P4_source_files” creates an _alias_!!!
        if JSON_input_specified:
            pathnames_to_check.append(opts.json_source)

        for pathname in pathnames_to_check:
            if not os.path.isfile(pathname):
                print('\nERROR: the input file "{}" does not exist.'.
                      format(pathname), file=sys.stderr)
                error_count += 1

    elif not any_input_specified:
        use_a_dummy_P4_pseudoPathname = True



    if error_count != 0:
        print("\nSorry; {} error{} found while analyzing the command inputs."
              "  Aborting the P4 compiler driver.\n".
              format(error_count, s_and_were_or_just_was(error_count)))
        sys.exit(min(255, error_count))
        ### maximum of 255: being extra-careful here, just in case “error_count” is a positive integer multiple of 256



    ### A reminder: by this point, “error_count” _must_ equal zero, due to the “sys.exit” call immediately above,
    ###   so we can make sanity-checking assertions in the following code without double-checking “error_count” first.



    ### Figure out what we need to pass to the backend as “opts.source_file”,
    ###   since that is the interface contract between the driver and backends as of this writing.
    ### Do some sanity checking along the way (e.g. assertions).

    string_to_pass_as___source_file = ""
    ### the preceding line: _intentionally_ initializing with a value that Python considers false

    if JSON_input_specified:
        assert 0 == len(opts.P4_source_files)
        assert not use_a_dummy_P4_pseudoPathname
        string_to_pass_as___source_file = opts.json_source

    if P4_input_or_inputs_specified:
        assert 1 == len(opts.P4_source_files)
        assert not JSON_input_specified
        string_to_pass_as___source_file = opts.P4_source_files[0]

    if use_a_dummy_P4_pseudoPathname:
        assert 0 == len(opts.P4_source_files)
        assert not any_input_specified
        assert not JSON_input_specified
        assert not P4_input_or_inputs_specified
        string_to_pass_as___source_file = "dummy.p4"



    ### The following is admittedly somewhat hackish, but was the only way
    ###   Abe could figure out how to set the appropriate attribute for the
    ###   interface contract with the back end while maintaining the
    ###   established norm that “opts” is of type “argparse.Namespace”,
    ###   as it was before Abe touched this file.
    opts.__setattr__("source_file", string_to_pass_as___source_file)



    # set all configuration and command line options for backend
    backend.process_command_line_options(opts)

    # run all commands
    rc = backend.run()
    sys.exit(rc)

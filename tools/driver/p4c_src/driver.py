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
import os
import shlex, subprocess
import sys

import p4c_src.util as util

class BackendDriver:
    """A class that has a list of passes that need to be run.  Each
    backend configures the commands that wants to be run.

    Backends may instantiate this class and override the processing of
    command line options.

    Each pass builds a command line that is invoked as a separate
    process.  Each pass also allows invoking a pre and post processing
    step to setup and cleanup after each command.

    """

    def __init__(self, target, arch, argParser = None):
        self._target = target
        self._arch = arch
        self._backend = target + '-' + arch
        self._commands = {}
        self._commandsEnabled = []
        self._preCmds = {}
        self._postCmds = {}
        self._argParser = argParser
        self._argGroup = None
        # options
        self._dry_run = False
        self._output_directory = "./"
        self._source_filename = None
        self._source_basename = None
        self._verbose = False

    def __str__(self):
        return self._backend

    def add_command(self, cmd_name, cmd):
        """ Add a command

        If the command was previously set, it is overwritten
        """
        if cmd_name in self._commands:
            print >> sys.stderr, "Warning: overwriting command", cmd_name
        self._commands[cmd_name] = []
        self._commands[cmd_name].append(cmd)

    def add_command_option(self, cmd_name, option):
        """ Add an option to a command
        """
        if cmd_name not in self._commands:
            if self._verbose:
                print >> sys.stderr, "Command", "'" + cmd_name + "'", \
                    "was not set for target", self._backend
            return
        self._commands[cmd_name].append(option)

    def add_command_line_options(self):
        """ Method for derived classes to add options to the parser
        """
        self._argGroup = self._argParser.add_argument_group(title = self._backend)


    def process_command_line_options(self, opts):
        """ Process all command line options
        """
        self._dry_run = opts.dry_run
        self._verbose = opts.debug
        self._output_directory = opts.output_directory
        self._source_filename = opts.source_file
        self._source_basename = os.path.splitext(os.path.basename(opts.source_file))[0]

        # set preprocessor options
        if 'preprocessor' in self._commands:
            for option in opts.preprocessor_options:
                self.add_command_option('preprocessor', option)

        # set compiler options.
        for option in opts.compiler_options:
            self.add_command_option('compiler', option)

        # set debug info
        if opts.debug_info:
            for c in self._commands:
                if c == 'assembler' or c == 'compiler' or c == 'linker':
                    self.add_command_option(c, "-g")

        # set assembler options
        if 'assembler' in self._commands:
            for option in opts.assembler_options:
                self.add_command_option('assembler', option)

        # set linker options
        if 'linker' in self._commands:
            for option in opts.linker_options:
                self.add_command_option('linker', option)

        # append to the list of defines
        for d in opts.preprocessor_defines:
            self.add_command_option('preprocessor', "-D"+d)
            self.add_command_option('compiler', "-D"+d)

        # Preserve comments: -C
        # Unix and std C keywords should be allowed in P4 (-undef and -nostdinc)
        # Allow using ' for constants rather than delimiters for strings (-x assembler-with-cpp)
        self.add_command_option('preprocessor', '-C -undef -nostdinc -x assembler-with-cpp')

        # default search path
        if opts.language == 'p4-16':
            self.add_command_option('preprocessor',
                                        "-I {}".format(os.environ['P4C_16_INCLUDE_PATH']))
            self.add_command_option('compiler',
                                    "-I {}".format(os.environ['P4C_16_INCLUDE_PATH']))
        else:
            self.add_command_option('preprocessor',
                                   "-I {}".format(os.environ['P4C_14_INCLUDE_PATH']))
            self.add_command_option('compiler',
                                   "-I {}".format(os.environ['P4C_14_INCLUDE_PATH']))

        # append search path
        for path in opts.search_path:
            self.add_command_option('preprocessor', "-I")
            self.add_command_option('preprocessor', path)
            self.add_command_option('compiler', "-I")
            self.add_command_option('compiler', path)

        # set p4 version
        if opts.language == 'p4-16':
            self.add_command_option('compiler', "--p4v=16")
        else:
            self.add_command_option('compiler', "--p4v=14")

        # P4Runtime options
        if opts.p4runtime_file:
            print >> sys.stderr, "'--p4runtime-file' and '--p4runtime-format'", \
                "are deprecated, consider using '--p4runtime-files'"
            self.add_command_option('compiler',
                                    "--p4runtime-file {}".format(opts.p4runtime_file))
            self.add_command_option('compiler',
                                    "--p4runtime-format {}".format(opts.p4runtime_format))

        if opts.p4runtime_files:
            self.add_command_option('compiler',
                                    "--p4runtime-files {}".format(opts.p4runtime_files))

        # set developer options
        if (os.environ['P4C_BUILD_TYPE'] == "DEVELOPER"):
            for option in opts.log_levels:
                self.add_command_option('compiler', "-T{}".format(option))
            if opts.passes:
                self.add_command_option('compiler', "--top4 {}".format(",".join(opts.passes)))
            if opts.debug:
                self.add_command_option('compiler', "-vvv")
            if opts.dump_dir:
                self.add_command_option('compiler', "--dump {}".format(opts.dump_dir))
            if opts.json:
                self.add_command_option('compiler', "--toJSON {}".format(opts.json))
            if opts.pretty_print:
                self.add_command_option('compiler', "--pp {}".format(opts.pretty_print))

        if (os.environ['P4C_BUILD_TYPE'] == "DEVELOPER") and \
           'assembler' in self._commands and opts.debug:
                self.add_command_option('assembler', "-vvv")

        # handle mode flags
        if opts.run_preprocessor_only:
            self.enable_commands(['preprocessor'])
        elif opts.skip_preprocessor:
            self.disable_commands(['preprocessor'])
        elif opts.run_till_assembler:
            self.enable_commands(['preprocessor', 'compiler'])
        elif opts.run_all:
            # this is the default, each backend driver is supposed to enable all
            # its commands and the order in which they execute
            pass

    def enable_commands(self, cmdsEnabled):
        """
        Defines the order in which the steps are executed and which commands
        are going to run
        """
        newCmds = [c for c in cmdsEnabled if c in self._commands]
        if len(newCmds) > 0:
            self._commandsEnabled = newCmds

    def disable_commands(self, cmdsDisabled):
        """
        Disables the commands in cmdsDisabled
        """
        for c in cmdsDisabled:
            if c in self._commandsEnabled:
                self._commandsEnabled.remove(c)

    def runCmd(self, step, cmd):
        """
        Run a command, capture its output and print it
        Also exit with the command error code if failed
        """
        if self._dry_run:
            print '{}:\n{}'.format(step, ' '.join(cmd))
            return 0

        args = shlex.split(" ".join(cmd))
        try:
            p = subprocess.Popen(args)
        except:
            import traceback
            print >> sys.stderr, "error invoking {}".format(" ".join(cmd))
            print >> sys.stderr, traceback.format_exc()
            return 1

        if self._verbose: print 'running {}'.format(' '.join(cmd))
        p.communicate() # now wait
        return p.returncode


    def preRun(self, cmd_name):
        """
        Preamble to a command to setup anything needed
        """
        if cmd_name not in self._preCmds:
            return # nothing to do

        cmds = self._preCmds[cmd_name]
        for c in cmds:
            rc = self.runCmd(cmd_name, c)
            if rc != 0:
                sys.exit(rc)

    def postRun(self, cmd_name):
        """
        Postamble to a command to cleanup
        """
        if cmd_name not in self._postCmds:
            return # nothing to do

        cmds = self._postCmds[cmd_name]
        rc = 0
        for c in cmds:
            rc += self.runCmd(cmd_name, c)
            # we will continue to run post commands even if some fail
            # so that we do all the cleanup
        return rc # \TODO should we fail on this or not?

    def run(self):
        """
        Run the set of commands required by this driver
        """

        # set output directory
        if not os.path.exists(self._output_directory):
            os.makedirs(self._output_directory)

        for c in self._commandsEnabled:

            # run the setup for the command
            self.preRun(c)

            # run the command
            cmd = self._commands[c]
            if cmd[0].find('/') != 0 and (util.find_bin(cmd[0]) == None):
                print >> sys.stderr, "{}: command not found".format(cmd[0])
                sys.exit(1)

            rc = self.runCmd(c, cmd)

            # run the cleanup whether the command succeeded or failed
            postrc = self.postRun(c)

            # if the main command failed, stop and return its error code so that
            # backends that override run can chose what to do on error
            if rc != 0:
                return rc

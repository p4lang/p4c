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
    """A class that has a list of passes that need to be run.
    Each backend instantiates this class and configures the commands
    that wants to be run.

    \TODO: the main program assumes that preprocessor, assembler,
    compiler, and linker are predefined commands. We should move those
    out of main and let the backend class configure its commands using
    the options.

    Each pass builds a command line that is invoked as a separate
    process.  Each pass also allows invoking a pre and post processing
    step to setup and cleanup after each command.

    """

    def __init__(self, backendName):
        self._backend = backendName
        self._commands = {}
        self._commandsEnabled = []
        self._preCmds = {}
        self._postCmds = {}

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
            print >> sys.stderr, "Command", "'" + cmd_name + "'", \
                "was not set for target", self._backend
            return
        self._commands[cmd_name].append(option)

    def enable_commands(self, cmdsEnabled):
        """
        Defines the order in which the steps are executed and which commands
        are going to run
        """
        self._commandsEnabled = cmdsEnabled

    def disable_commands(self, cmdsDisabled):
        """
        Disables the commands in cmdsDisabled
        """
        for c in cmdsDisabled:
            if c in self._commandsEnabled:
                self._commandsEnabled.remove(c)

    def runCmd(self, step, cmd, opts):
        """
        Run a command, capture its output and print it
        Also exit with the command error code if failed
        """
        # only dry-run
        if opts.dry_run:
            print '{}:\n{}'.format(step, ' '.join(cmd))
            return 0

        args = shlex.split(" ".join(cmd))
        try:
            p = subprocess.Popen(args, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        except:
            import traceback
            print >> sys.stderr, "error invoking {}".format(" ".join(cmd))
            print >> sys.stderr, traceback.format_exc()
            return 1

        if opts.debug: print 'running {}'.format(' '.join(cmd))
        out, err = p.communicate() # now wait
        if len(out) > 0: print out
        if len(err) > 0: print >> sys.stderr, err
        return p.returncode


    def preRun(self, cmd_name, opts):
        """
        Preamble to a command to setup anything needed
        """
        if cmd_name not in self._preCmds:
            return # nothing to do

        cmd = self._preCmds[cmd_name]
        rc = self.runCmd(cmd_name, cmd, opts)
        if rc != 0:
            sys.exit(rc)

    def postRun(self, cmd_name, opts):
        """
        Postamble to a command to cleanup
        """
        if cmd_name not in self._postCmds:
            return # nothing to do

        cmd = self._postCmds[cmd_name]
        rc = self.runCmd(cmd_name, cmd, opts)
        if rc != 0:
            sys.exit(rc)

    def run(self, opts):
        """
        Run the set of commands required by this driver
        """

        # set output directory
        if not os.path.exists(opts.output_directory):
            os.makedirs(opts.output_directory)

        for c in self._commandsEnabled:

            # run the setup for the command
            self.preRun(c, opts)

            # run the command
            cmd = self._commands[c]
            if cmd[0].find('/') != 0 and (util.find_bin(cmd[0]) == None):
                print >> sys.stderr, "{}: command not found".format(cmd[0])
                sys.exit(1)

            rc = self.runCmd(c, cmd, opts)

            # run the cleanup whether the command succeeded or failed
            self.postRun(c, opts)

            # if the main command failed, exit with its error code
            if rc != 0:
                sys.exit(rc)

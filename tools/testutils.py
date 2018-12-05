#!/usr/bin/env python2
# Copyright 2013-present Barefoot Networks, Inc.
# Copyright 2018 VMware, Inc.
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

""" Defines helper functions for a general testing framework. Used by multiple
    Python testing scripts in the backends folder."""

from __future__ import print_function
import subprocess
from subprocess import Popen
from threading import Timer
import sys
import os

TIMEOUT = 10 * 60
SUCCESS = 0
FAILURE = 1
SKIPPED = 2  # used occasionally to indicate that a test was not executed


def is_err(p4filename):
    """ True if the filename represents a p4 program that should fail. """
    return "_errors" in p4filename


def report_err(file, *message):
    """ Write message to given file, report to stderr if verbose """
    print("***", file=sys.stderr, *message)

    if (file and file != sys.stderr):
        err_file = open(file, "a+")
        print("***", file=err_file, *message)
        err_file.close()


def report_output(file, verbose, *message):
    """ Write message to given file, report to stdout if verbose """
    if (verbose):
        print(file=sys.stdout, *message)

    if (file and file != sys.stdout):
        out_file = open(file, "a+")
        print("", file=out_file, *message)
        out_file.close()


def byte_to_hex(byteStr):
    """ Convert byte sequences to a hex string. """
    return ''.join(["%02X " % ord(x) for x in byteStr]).strip()


def hex_to_byte(hexStr):
    """ Convert hex strings to bytes. """
    bytes = []
    hexStr = ''.join(hexStr.split(" "))
    for i in range(0, len(hexStr), 2):
        bytes.append(chr(int(hexStr[i:i + 2], 16)))
    return ''.join(bytes)


def compare_pkt(outputs, expected, received):
    """  Compare two given byte sequences and check if they are the same.
         Report errors if this is not the case. """
    received = ''.join(byte_to_hex(str(received)).split()).upper()
    expected = ''.join(expected.split()).upper()
    if len(received) < len(expected):
        report_err(outputs["stderr"], "Received packet too short",
                   len(received), "vs", len(expected))
        return FAILURE
    for i in range(0, len(expected)):
        if expected[i] == "*":
            continue
        if expected[i] != received[i]:
            report_err(outputs["stderr"], "Received packet ", received)
            report_err(outputs["stderr"], "Packet different at position", i,
                       ": expected", expected[i], ", received", received[i])
            report_err(outputs["stderr"], "Expected packet ", expected)
            return FAILURE
    return SUCCESS


def open_process(verbose, args, outputs):
    """ Run the given arguments as a subprocess. Time out after TIMEOUT
        seconds and report failures or stdout. """
    report_output(outputs["stdout"],
                  verbose, "Writing", args)
    proc = None
    if outputs["stderr"] is not None:
        try:
            proc = Popen(args, stdout=subprocess.PIPE, shell=True,
                         stdin=subprocess.PIPE, stderr=subprocess.PIPE,
                         universal_newlines=True)
        except OSError as e:
            report_err(outputs["stderr"], "Failed executing: ", e)
    if proc is None:
        # Never even started
        report_err(outputs["stderr"], "Process failed to start")
    return proc


def run_process(verbose, proc, timeout, outputs, errmsg):
    def kill(process):
        process.kill()
    timer = Timer(TIMEOUT, kill, [proc])
    try:
        timer.start()
        out, err = proc.communicate()
    finally:
        timer.cancel()
    if out:
        msg = ("\n########### PROCESS OUTPUT BEGIN:\n"
               "%s########### PROCESS OUTPUT END\n" % out)
        report_output(outputs["stdout"], verbose, msg)
    if proc.returncode != SUCCESS:
        report_err(outputs["stderr"], "Error %d: %s\n%s" %
                   (proc.returncode, errmsg, err))
    else:
        # Also report non fatal warnings in stdout
        if err:
            report_err(outputs["stderr"], err)
    return proc.returncode


def run_timeout(verbose, args, timeout, outputs, errmsg):
    proc = open_process(verbose, args, outputs)
    if proc is None:
        return FAILURE
    report_output(outputs["stdout"],
                  verbose, "Executing", args)
    return run_process(verbose, proc, timeout, outputs, errmsg)


def check_root():
    """ This function returns False if the user does not have root privileges.
        Caution: Only works on Unix systems """
    return (os.getuid() == 0)

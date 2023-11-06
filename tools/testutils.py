#!/usr/bin/env python3
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

import logging
import os
import random
import shutil
import signal
import socket
import subprocess
import threading
from pathlib import Path
from typing import NamedTuple, Optional

# Set up logging.
log = logging.getLogger(__name__)

TIMEOUT: int = 10 * 60
SUCCESS: int = 0
FAILURE: int = 1
# SKIPPED is used to indicate that a test was not executed.
SKIPPED: int = 999


import scapy.packet


class LogPipe(threading.Thread):
    """A log utility class that allows subprocesses to directly write into a log.
    Derived from https://codereview.stackexchange.com/a/17959."""

    def __init__(self, level: int):
        """Setup the object with a logger and a loglevel
        and start the thread
        """
        threading.Thread.__init__(self)
        self.daemon: bool = False
        self.level: int = level
        self.fd_read, self.fd_write = os.pipe()
        self.pipe_reader = os.fdopen(self.fd_read)
        # We capture what we log to this string.
        self.out: str = ""
        self.start()

    def fileno(self) -> int:
        """Return the write file descriptor of the pipe"""
        return self.fd_write

    def run(self) -> None:
        """Run the thread, logging and record everything."""
        for line in iter(self.pipe_reader.readline, ""):
            log.log(self.level, line.strip("\n"))
            self.out += line
        self.pipe_reader.close()

    def close(self) -> None:
        """Close the write end of the pipe."""
        os.close(self.fd_write)


class ProcessResult(NamedTuple):
    """Simplified process result class. Only contains one output string and the result."""

    output: str
    returncode: int


def is_err(p4filename: str) -> bool:
    """True if the filename represents a p4 program that should fail."""
    return "_errors" in p4filename


def hex_to_byte(hex_str: str) -> str:
    """Convert hex strings to bytes."""
    byte_vals = []
    hex_str = "".join(hex_str.split(" "))
    for i in range(0, len(hex_str), 2):
        byte_vals.append(chr(int(hex_str[i : i + 2], 16)))
    return "".join(byte_vals)


def compare_pkt(expected: str, received: scapy.packet.Packet) -> int:
    """Compare two given byte sequences and check if they are the same.
    Report errors if this is not the case."""

    # If the expected packet string ends with a '$' it means that the packets are only equal,
    # if they are the exact same length.
    strict_length_check = False
    if expected[-1] == '$':
        strict_length_check = True
        expected = expected[:-1]

    received = received.build().hex().upper()
    expected = "".join(expected.split()).upper()
    if strict_length_check and len(received) > len(expected):
        log.error(
            "Received packet too long %s vs %s (in units of hex digits)",
            len(received),
            len(expected),
        )
        return FAILURE
    if len(received) < len(expected):
        log.error("Received packet too short %s vs %s", len(received), len(expected))
        return FAILURE
    for idx, val in enumerate(expected):
        if val == "*":
            continue
        if val != received[idx]:
            log.error("Received packet\n %s ", received)
            log.error(
                "Packet different at position %s: expected\n%s\n\nreceived\n%s",
                idx,
                val,
                received[idx],
            )
            log.error("Expected packet\n %s", expected)
            return FAILURE
    return SUCCESS


def is_tcp_port_in_use(addr: str, port: int) -> bool:
    """Helper function to check whether a given TCP port number is in use on this system."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tcp_socket:
        return tcp_socket.connect_ex((addr, port)) == 0


def pick_tcp_port(addr: str, default_port: int) -> int:
    """Helper function to check pick a free TCP port."""
    while True:
        if not is_tcp_port_in_use(addr, default_port):
            break
        default_port = random.randrange(1024, 65535)
    return default_port


def open_process(args: str, **extra_args) -> Optional[subprocess.Popen]:
    """Start the given argument string as a subprocess and return the handle to the process.
    @param extra_args is forwarded to the subprocess.communicate command"""
    log.info("Writing %s", args)
    proc = None
    output_args = {
        "stdout": subprocess.PIPE,
        "stdin": subprocess.PIPE,
        "stderr": subprocess.PIPE,
        "universal_newlines": True,
        "preexec_fn": os.setsid,
    }
    output_args = {**output_args, **extra_args}

    # Only split the arguments if the shell option is not present.
    if not ("shell" in extra_args and extra_args["shell"]):
        args = args.split()
        # Sanitize all empty strings.
        args = list(filter(None, args))

    try:
        proc = subprocess.Popen(args, **output_args)
    except OSError as exception:
        log.error("Failed executing:\n%s", exception)
    if proc is None:
        # Never even started
        log.error("Process failed to start")
    return proc


def run_process(proc: subprocess.Popen, **extra_args) -> subprocess.Popen:
    """Wait for the given process to finish. Report failures to stderr. @param extra_args is
    forwarded to the subprocess.communicate command."""
    try:
        out, err = proc.communicate(**extra_args)
    except subprocess.TimeoutExpired as exception:
        log.error("Command '%s' timed out.", exception.cmd)
        out = exception.stdout
        err = exception.stderr
    if out:
        msg = f"\n########### PROCESS OUTPUT BEGIN:\n{out}########### PROCESS OUTPUT END\n"
        log.info(msg)
    if proc.returncode != SUCCESS:
        log.error("Error %s:\n%s", proc.returncode, err)
    else:
        # Also report non fatal warnings in stdout
        if err:
            log.error(err)
    return proc


def exec_process(args: str, **extra_args) -> ProcessResult:
    """Run the given argument string as a subprocess. Time out after TIMEOUT
    seconds and report failures to stderr. @param extra_args is forwarded to the subprocess.run
    command."""
    if not isinstance(args, str):
        log.error("Input must be a string. Received %s.", args)
        return ProcessResult(f"Input must be a string. Received {args}.", FAILURE)

    log.info("Executing command: %s", args)
    output_args = {"timeout": TIMEOUT, "universal_newlines": True}
    output_args = {**output_args, **extra_args}

    # Only split the arguments if the shell option is not present.
    if not ("shell" in extra_args and extra_args["shell"]):
        args = args.split()
        # Sanitize all empty strings.
        args = list(filter(None, args))

    # Set up log pipes for both stdout and stderr.
    outpipe: Optional[LogPipe] = None
    errpipe: Optional[LogPipe] = None
    if "capture_output" not in extra_args:
        if "stdout" not in extra_args:
            outpipe = LogPipe(logging.INFO)
            output_args["stdout"] = outpipe
        if "stderr" not in extra_args:
            errpipe = LogPipe(logging.WARNING)
            output_args["stderr"] = errpipe
    try:
        result = subprocess.run(args, check=True, **output_args)
        if outpipe:
            out = outpipe.out
        else:
            out = result.stdout
        returncode = result.returncode
    except subprocess.CalledProcessError as exception:
        if errpipe:
            out = errpipe.out
        returncode = exception.returncode
        cmd = exception.cmd
        # Rejoin the list for better readability.
        if isinstance(cmd, list):
            cmd = " ".join(cmd)
        log.error('Error %s when executing "%s".', returncode, cmd)
    except subprocess.TimeoutExpired as exception:
        if errpipe:
            out = errpipe.out
        else:
            out = str(exception.stderr)
        returncode = FAILURE
        cmd = exception.cmd
        # Rejoin the list for better readability.
        if isinstance(cmd, list):
            cmd = " ".join(cmd)
        log.error("Timed out when executing %s.", cmd)
    finally:
        if "capture_output" not in extra_args:
            if outpipe:
                outpipe.close()
            if errpipe:
                errpipe.close()
    return ProcessResult(out, returncode)


def kill_proc_group(proc: subprocess.Popen) -> None:
    """Kill a process group and all processes associated with this group."""
    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)


def check_root() -> bool:
    """This function returns False if the user does not have root privileges.
    Caution: Only works on Unix systems"""
    return os.getuid() == 0


def check_if_file(input_path_str: str) -> Optional[Path]:
    """Checks if a path is a file and converts the input
    to an absolute path"""
    if not input_path_str:
        log.error("input_path is None")
        return None
    input_path = Path(input_path_str)
    if not input_path.exists():
        log.error("%s does not exist", input_path)
        return None
    if not input_path.is_file():
        log.error("%s is not a file", input_path)
        return None
    return Path(input_path.absolute())


def check_if_dir(input_path_str: str) -> Optional[Path]:
    """Checks if a path is an actual directory and converts the input
    to an absolute path"""
    if not input_path_str:
        log.error("input_path is None")
        return None
    input_path = Path(input_path_str)
    if not input_path.exists():
        log.error("%s does not exist", input_path)
        return None
    if not input_path.is_dir():
        log.error("%s is not a directory", input_path)
        return None
    return Path(input_path.absolute())


def check_and_create_dir(directory: Path) -> None:
    """Create the folder if it does not exit."""
    if directory and not directory.exists():
        log.info("Folder %s does not exist! Creating...", directory)
        directory.mkdir(parents=True, exist_ok=True)


def del_dir(directory: str) -> None:
    """Delete a directory and all its children.
    TODO: Convert to Path input."""
    try:
        shutil.rmtree(directory, ignore_errors=True)
    except OSError as exception:
        log.error(
            "Could not delete directory, reason:\n%s - %s.",
            exception.filename,
            exception.strerror,
        )


def copy_file(src, dst):
    """Copy a file or a list of files to a destination."""
    try:
        if isinstance(src, list):
            for src_file in src:
                shutil.copy2(src_file, dst)
        else:
            shutil.copy2(src, dst)
    except shutil.SameFileError:
        # Exceptions are okay here.
        pass

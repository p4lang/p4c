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

import subprocess
import logging
import sys
import socket
import random
import os
import shutil
import signal
from threading import Timer
from pathlib import Path
from typing import NamedTuple

# configure logging
log = logging.getLogger(__name__)

TIMEOUT = 10 * 60
SUCCESS = 0
FAILURE = 1
SKIPPED = 999  # used occasionally to indicate that a test was not executed


class ProcessResult(NamedTuple):
    """Simplified process result class. Only contains one output string and the result."""

    output: str
    returncode: int


def is_err(p4filename):
    """True if the filename represents a p4 program that should fail."""
    return "_errors" in p4filename


def hex_to_byte(hex_str):
    """Convert hex strings to bytes."""
    byte_vals = []
    hex_str = "".join(hex_str.split(" "))
    for i in range(0, len(hex_str), 2):
        byte_vals.append(chr(int(hex_str[i : i + 2], 16)))
    return "".join(byte_vals)


def compare_pkt(expected, received):
    """Compare two given byte sequences and check if they are the same.
    Report errors if this is not the case."""
    received = bytes(received).hex().upper()
    expected = "".join(expected.split()).upper()
    if len(received) < len(expected):
        log.error(f"Received packet too short {len(received)} vs {len(expected)}")
        return FAILURE
    for idx, val in enumerate(expected):
        if val == "*":
            continue
        if val != received[idx]:
            log.error(f"Received packet\n {received} ")
            log.error(
                f"Packet different at position {idx}: expected\n{val}\n\nreceived\n{received[idx]}"
            )
            log.error(f"Expected packet {expected}")
            return FAILURE
    return SUCCESS


def is_tcp_port_in_use(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        return s.connect_ex(("localhost", port)) == 0


def pick_tcp_port(default_port: int) -> int:
    # Pick an available port.
    port_used = True
    while port_used:
        default_port = random.randrange(1024, 65535)
        port_used = is_tcp_port_in_use(default_port)

    return default_port


def open_process(args):
    """Run the given arguments as a subprocess. Time out after TIMEOUT
    seconds and report failures or stdout."""
    log.info(f"Writing {args}")
    proc = None

    try:
        proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            shell=True,
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            preexec_fn=os.setsid,
        )
    except OSError as e:
        log.error(f"Failed executing:\n{e}")
    if proc is None:
        # Never even started
        log.error("Process failed to start")
    return proc


def run_process(proc, timeout, errmsg):
    def kill(process):
        process.kill()

    timer = Timer(timeout, kill, [proc])
    try:
        timer.start()
        out, err = proc.communicate()
    finally:
        timer.cancel()
    if out:
        msg = "\n########### PROCESS OUTPUT BEGIN:\n" f"{out}########### PROCESS OUTPUT END\n"
        log.info(msg)
    if proc.returncode != SUCCESS:
        log.error(f"Error {proc.returncode}: {errmsg}\n{err}")
    else:
        # Also report non fatal warnings in stdout
        if err:
            log.error(err)
    return proc.returncode


def exec_process(args, errmsg, timeout=TIMEOUT):
    log.debug(f"Executing command: {args}")
    output_args = {}
    if log.getEffectiveLevel() <= logging.INFO:
        output_args = {"stdout": sys.stdout, "stderr": sys.stdout}
    else:
        output_args = {"stdout": subprocess.PIPE, "stderr": sys.stdout}
    try:
        result = subprocess.run(
            args, shell=True, timeout=timeout, universal_newlines=True, check=True, **output_args
        )
    except subprocess.CalledProcessError as e:
        out = e.stdout
        returncode = e.returncode
        log.error(f"Error {returncode} when executing {e.cmd}:\n{errmsg}\n{out}")
    else:
        out = result.stdout
        returncode = result.returncode
    if log.getEffectiveLevel() <= logging.INFO:
        if out:
            log.info("########### PROCESS OUTPUT BEGIN:\n" f"{out}########### PROCESS OUTPUT END")
    return ProcessResult(out, returncode)


def kill_proc_group(proc):
    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)


def check_root():
    """This function returns False if the user does not have root privileges.
    Caution: Only works on Unix systems"""
    return os.getuid() == 0


class PathError(RuntimeError):
    pass


def check_if_file(input_path):
    """Checks if a path is a file and converts the input
    to an absolute path"""
    if not input_path:
        log.error(sys.stderr, "input_path is None")
        sys.exit(1)
    input_path = Path(input_path)
    if not input_path.exists():
        log.error(sys.stderr, f"{input_path} does not exist")
        sys.exit(1)
    if not input_path.is_file():
        log.error(sys.stderr, f"{input_path} is not a file")
        sys.exit(1)
    return Path(input_path.absolute())


def check_if_dir(input_path):
    """Checks if a path is an actual directory and converts the input
    to an absolute path"""
    if not input_path:
        log.error(sys.stderr, "input_path is None")
        sys.exit(1)
    input_path = Path(input_path)
    if not input_path.exists():
        log.error(sys.stderr, f"{input_path} does not exist")
        sys.exit(1)
    if not input_path.is_dir():
        log.error(sys.stderr, f"{input_path} is not a directory")
        sys.exit(1)
    return Path(input_path.absolute())


def check_and_create_dir(directory):
    # create the folder if it does not exit
    if not directory == "" and not os.path.exists(directory):
        log.info(f"Folder {directory} does not exist! Creating...")
        directory.mkdir(parents=True, exist_ok=True)


def del_dir(directory):
    try:
        shutil.rmtree(directory, ignore_errors=True)
    except OSError as e:
        log.error(f"Could not delete directory, reason:\n{e.filename} - {e.strerror}.")


def copy_file(src, dst):
    try:
        if isinstance(src, list):
            for src_file in src:
                shutil.copy2(src_file, dst)
        else:
            shutil.copy2(src, dst)
    except shutil.SameFileError:
        # this is fine
        pass

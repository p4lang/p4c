#!/usr/bin/env python3

import argparse
import subprocess
import os
import sys
import socket
import random
import tempfile
from pathlib import Path
from datetime import datetime

FILE_DIR = Path(__file__).parent.resolve()
TOOLS_PATH = FILE_DIR.joinpath("../../tools")
sys.path.append(str(TOOLS_PATH))

BRIDGE_PATH = FILE_DIR.joinpath("../ebpf/targets")
sys.path.append(str(BRIDGE_PATH))
from ebpfenv import Bridge
import testutils

PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir", help="the root directory of " "the compiler source tree")
PARSER.add_argument("p4_file", help="the p4 file to process")
PARSER.add_argument(
    "-tf",
    "--testfile",
    dest="testfile",
    help="The path for the ptf py file for this test.",
)
PARSER.add_argument(
    "-td",
    "--testdir",
    dest="testdir",
    help="The location of the test directory. ",
)


def is_port_in_use(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        return s.connect_ex(("localhost", port)) == 0


class Options:
    def __init__(self):
        self.p4_file: Path = None  # File that is being compiled
        self.testfile: Path = None  # path to ptf test file that is used
        # Actual location of the test framework
        self.testdir: Path = None
        self.rootDir: Path = "."


def create_bridge():
    testutils.report_output(
        sys.stdout,
        "---------------------- Start creating of bridge ----------------------",
    )
    random.seed(datetime.now().timestamp())
    outputs = {}
    outputs["stdout"] = sys.stdout
    outputs["stderr"] = sys.stderr
    bridge = Bridge(random.random(), outputs, True)
    result = bridge.create_virtual_env(8)
    if result != testutils.SUCCESS:
        bridge.ns_del()
        testutils.report_err(
            sys.stderr,
            "---------------------- End creating of bridge with errors ----------------------",
        )
        sys.exit(1)
    testutils.report_output(
        sys.stdout, "---------------------- Bridge created ----------------------"
    )
    return bridge


def open_proc(bridge):
    proc = bridge.ns_proc_open()
    if not proc:
        bridge.ns_del()
        testutils.report_err(
            sys.stderr,
            "---------------------- Unable to open bash process in the namespace ----------------------",
        )
        sys.exit(1)
    return proc


def kill_process(proc):
    # kill process
    os.kill(proc.pid, 15)
    os.kill(proc.pid, 9)


def create_runtime(options, json_name, info_name):
    testutils.report_output(
        sys.stdout, "---------------------- Start p4c-bm2-ss ----------------------"
    )
    p4c_bm2_ss = (
        f"{options.rootDir}/build/p4c-bm2-ss --target bmv2 --arch v1model "
        f"--p4runtime-files {info_name} {options.p4_file} -o {json_name}"
    )
    p = subprocess.Popen(
        p4c_bm2_ss, shell=True, stdin=subprocess.PIPE, universal_newlines=True
    )

    p.communicate()
    if p.returncode != 0:
        testutils.report_err(
            sys.stderr,
            "---------------------- End p4c-bm2-ss with errors ----------------------",
        )
        raise SystemExit("p4c-bm2-ss ended with errors")


def _run_proc_in_background(bridge, cmd):
    namedCmd = bridge.get_ns_prefix() + " " + cmd
    return subprocess.Popen(
        namedCmd, shell=True, stdin=subprocess.PIPE, universal_newlines=True
    )


def run_simple_switch_grpc(bridge, thrift_port, grpc_port):
    proc = open_proc(bridge)
    # Remove the log file.
    removeLogFileCmd = "/bin/rm -f ss-log.txt"
    bridge.ns_exec(removeLogFileCmd)

    testutils.report_output(
        sys.stdout,
        "---------------------- Start simple_switch_grpc ----------------------",
    )

    simple_switch_grpc = (
        f"simple_switch_grpc --thrift-port {thrift_port} --log-console -i 0@0 "
        f"-i 1@1 -i 2@2 -i 3@3 -i 4@4 -i 5@5 -i 6@6 -i 7@7 --no-p4 "
        f"-- --grpc-server-addr localhost:{grpc_port}"
    )

    switchProc = _run_proc_in_background(bridge, simple_switch_grpc)
    if switchProc.poll() is not None:
        kill_process(switchProc)
        testutils.report_err(
            sys.stderr,
            "---------------------- End simple_switch_grpc with errors ----------------------",
        )
        raise SystemExit("simple_switch_grpc ended with errors")

    return switchProc


def run_ptf(bridge, grpc_port, json_name, info_name):
    testutils.report_output(
        sys.stdout, "---------------------- Start ptf ----------------------"
    )
    # Add the file location to the python path.
    pypath = FILE_DIR
    # Show list of the tests
    testLostCmd = f"ptf --pypath {pypath} --test-dir {options.testdir} --list"
    bridge.ns_exec(testLostCmd)

    ifaces = "-i 0@br_0 -i 1@br_1 -i 2@br_2 -i 3@br_3 -i 4@br_4 -i 5@br_5 -i 6@br_6 -i 7@br_7"
    test_params = (
        f"grpcaddr='localhost:{grpc_port}';p4info='{info_name}';config='{json_name}';"
    )
    ptf = f'ptf --verbose --pypath {pypath} {ifaces} --test-params=\\"{test_params}\\" --test-dir {options.testdir}'
    return bridge.ns_exec(ptf)


def pick_port():
    # Pick an available port.
    grpc_port_used = True
    grpc_port = 28000
    while grpc_port_used:
        grpc_port = random.randrange(1024, 65535)
        grpc_port_used = is_port_in_use(grpc_port)
    thrift_port_used = True
    thrift_port = 28000
    while thrift_port_used or thrift_port == grpc_port:
        thrift_port = random.randrange(1024, 65535)
        thrift_port_used = is_port_in_use(thrift_port)

    return [thrift_port, grpc_port]


def run_test(options):
    """Define the test environment and compile the p4 target
    Optional: Run the generated model"""
    assert isinstance(options, Options)
    bridge = create_bridge()
    test_name = Path(options.p4_file.name)
    json_name = options.testdir.joinpath(test_name.with_suffix(".json"))
    info_name = options.testdir.joinpath(test_name.with_suffix(".p4info.txt"))
    # Copy the test file into the test folder so that it can be picked up by PTF.
    testutils.copy_file(options.testfile, options.testdir)
    create_runtime(options, json_name, info_name)
    thrift_port, grpc_port = pick_port()

    switch_proc = run_simple_switch_grpc(bridge, thrift_port, grpc_port)
    result = run_ptf(bridge, grpc_port, json_name, info_name)
    bridge.ns_del()
    return result


if __name__ == "__main__":

    if not testutils.check_root():
        errmsg = "This script requires root privileges; Exiting."
        testutils.report_err(sys.stderr, errmsg)
        sys.exit(1)
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()
    options = Options()

    options.p4_file = Path(testutils.check_if_file(args.p4_file))
    testfile = args.testfile
    if not testfile:
        testutils.report_output(
            sys.stdout, "No test file provided. Checking for file in folder."
        )
        testfile = options.p4_file.with_suffix(".py")
    options.testfile = Path(testutils.check_if_file(testfile))
    testdir = args.testdir
    if not testdir:
        testutils.report_output(
            sys.stdout, "No test directory provided. Generating temporary folder."
        )
        testdir = tempfile.mkdtemp(dir=os.path.abspath("./"))
        os.chmod(testdir, 0o744)
    options.testdir = Path(testdir)
    options.rootDir = Path(args.rootdir)
    # Run the test with the extracted options
    sys.exit(run_test(options))

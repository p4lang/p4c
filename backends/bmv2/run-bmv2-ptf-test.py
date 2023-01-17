#!/usr/bin/env python3

import argparse
import os
import sys
import random
import logging
import tempfile
from pathlib import Path
from datetime import datetime

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
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
    help="The location of the test directory.",
)
PARSER.add_argument(
    "-b",
    "--nocleanup",
    action="store_true",
    dest="nocleanup",
    help="Do not remove temporary results for failing tests.",
)
PARSER.add_argument(
    "-n",
    "--num-ifaces",
    default=8,
    dest="num_ifaces",
    help="How many virtual interfaces to create.",
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="WARNING",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)


class Options:
    def __init__(self):
        # File that is being compiled.
        self.p4_file = None
        # Path to ptf test file that is used.
        self.testfile = None
        # Actual location of the test framework.
        self.testdir = None
        # The base directory where tests are executed.
        self.rootdir = "."
        # The number of interfaces to create for this particular test.
        self.num_ifaces = 8


def create_bridge(num_ifaces):
    testutils.log.info(
        "---------------------- Start creating of bridge ----------------------",
    )
    random.seed(datetime.now().timestamp())
    bridge = Bridge(random.randint(0, sys.maxsize))
    result = bridge.create_virtual_env(num_ifaces)
    if result != testutils.SUCCESS:
        bridge.ns_del()
        testutils.log.error(
            "---------------------- End creating of bridge with errors ----------------------",
        )
        raise SystemExit("Unable to create the namespace environment.")
    testutils.log.info("---------------------- Bridge created ----------------------")
    return bridge


def compile_program(options, json_name, info_name):
    testutils.log.info("---------------------- Start p4c-bm2-ss ----------------------")
    compilation_cmd = (
        f"{options.rootdir}/build/p4c-bm2-ss --target bmv2 --arch v1model "
        f"--p4runtime-files {info_name} {options.p4_file} -o {json_name}"
    )
    _, returncode = testutils.exec_process(
        compilation_cmd, "Could not compile the P4 program", timeout=30
    )
    return returncode


def run_simple_switch_grpc(bridge, switchlog, grpc_port):
    thrift_port = testutils.pick_tcp_port(22000)
    testutils.log.info(
        "---------------------- Start simple_switch_grpc ----------------------",
    )
    simple_switch_grpc = (
        f"simple_switch_grpc --thrift-port {thrift_port} --log-file {switchlog} --log-flush -i 0@0 "
        f"-i 1@1 -i 2@2 -i 3@3 -i 4@4 -i 5@5 -i 6@6 -i 7@7 --no-p4 "
        f"-- --grpc-server-addr localhost:{grpc_port}"
    )
    bridge_cmd = bridge.get_ns_prefix() + " " + simple_switch_grpc
    switchProc = testutils.open_process(bridge_cmd)
    if switchProc is None:
        bridge.ns_del()
        raise SystemExit("simple_switch_grpc ended with errors")

    return switchProc


def run_ptf(bridge, grpc_port, testdir, json_name, info_name) -> int:
    testutils.log.info("---------------------- Start ptf ----------------------")
    # Add the file location to the python path.
    pypath = FILE_DIR
    # Show list of the tests
    testLostCmd = f"ptf --pypath {pypath} --test-dir {testdir} --list"
    returncode = bridge.ns_exec(testLostCmd)
    if returncode != testutils.SUCCESS:
        return returncode
    ifaces = "-i 0@br_0 -i 1@br_1 -i 2@br_2 -i 3@br_3 -i 4@br_4 -i 5@br_5 -i 6@br_6 -i 7@br_7"
    test_params = f"grpcaddr='localhost:{grpc_port}';p4info='{info_name}';config='{json_name}';"
    ptf = f'ptf --verbose --pypath {pypath} {ifaces} --test-params=\\"{test_params}\\" --test-dir {testdir}'
    returncode = bridge.ns_exec(ptf)
    return returncode


def run_test(options: Options) -> int:
    """Define the test environment and compile the P4 target
    Optional: Run the generated model"""
    test_name = Path(options.p4_file.name)
    json_name = options.testdir.joinpath(test_name.with_suffix(".json"))
    info_name = options.testdir.joinpath(test_name.with_suffix(".p4info.txt"))
    # Copy the test file into the test folder so that it can be picked up by PTF.
    testutils.copy_file(options.testfile, options.testdir)
    # Compile the P4 program.
    returncode = compile_program(options, json_name, info_name)
    if returncode != testutils.SUCCESS:
        return returncode
    # Create the virtual environment for the test execution.
    bridge = create_bridge(options.num_ifaces)
    # Pick available ports for the gRPC switch.
    grpc_port = testutils.pick_tcp_port(28000)
    switchlog = options.testdir.joinpath("switchlog")
    switch_proc = run_simple_switch_grpc(bridge, switchlog, grpc_port)
    # Run the PTF test and retrieve the result.
    result = run_ptf(bridge, grpc_port, options.testdir, json_name, info_name)
    if result != testutils.SUCCESS:
        # Terminate the switch process and emit its output in case of failure.
        testutils.kill_proc_group(switch_proc)
        testutils.log.error(f"#### Switch log\n{switchlog.with_suffix('.txt').read_text()}")
    bridge.ns_del()
    return result


def create_options(test_args) -> Options:
    options = Options()
    options.p4_file = Path(testutils.check_if_file(test_args.p4_file))
    testfile = test_args.testfile
    if not testfile:
        testutils.log.info("No test file provided. Checking for file in folder.")
        testfile = options.p4_file.with_suffix(".py")
    options.testfile = Path(testutils.check_if_file(testfile))
    testdir = test_args.testdir
    if not testdir:
        testutils.log.info("No test directory provided. Generating temporary folder.")
        testdir = tempfile.mkdtemp(dir=Path(".").absolute())
        # Generous permissions because the program is usually edited by sudo.
        os.chmod(testdir, 0o755)
    options.testdir = Path(testdir)
    options.rootdir = Path(test_args.rootdir)
    options.num_ifaces = args.num_ifaces

    # Configure logging.
    logging.basicConfig(
        filename=options.testdir.joinpath("testlog.log"),
        format="%(levelname)s:%(message)s",
        level=getattr(logging, test_args.log_level),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s:%(message)s"))
    logging.getLogger().addHandler(stderr_log)
    return options


if __name__ == "__main__":
    if not testutils.check_root():
        errmsg = "This script requires root privileges; Exiting."
        testutils.log.error(errmsg)
        sys.exit(1)
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()

    test_options = create_options(args)
    # Run the test with the extracted options
    test_result = run_test(test_options)
    if not (args.nocleanup or test_result != testutils.SUCCESS):
        testutils.log.info("Removing temporary test directory.")
        testutils.del_dir(test_options.testdir)
    sys.exit(test_result)

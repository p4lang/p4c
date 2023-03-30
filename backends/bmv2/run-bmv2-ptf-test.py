#!/usr/bin/env python3

import argparse
import logging
import os
import random
import sys
import tempfile
from datetime import datetime
from pathlib import Path

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
TOOLS_PATH = FILE_DIR.joinpath("../../tools")
sys.path.append(str(TOOLS_PATH))

BRIDGE_PATH = FILE_DIR.joinpath("../ebpf/targets")
sys.path.append(str(BRIDGE_PATH))
import testutils
from ebpfenv import Bridge

PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir", help="the root directory of the compiler source tree")
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

GRPC_PORT = 28000
THRIFT_PORT = 22000


class Options:
    """Options for this testing script. Usually correspond to command line inputs."""

    # File that is being compiled.
    p4_file: Path = Path(".")
    # Path to ptf test file that is used.
    testfile: Path = Path(".")
    # Actual location of the test framework.
    testdir: Path = Path(".")
    # The base directory where tests are executed.
    rootdir: Path = Path(".")
    # The number of interfaces to create for this particular test.
    num_ifaces = 8


def create_bridge(num_ifaces: int) -> Bridge:
    """Create a network namespace environment."""
    testutils.log.info(
        "---------------------- Creating a namespace ----------------------",
    )
    random.seed(datetime.now().timestamp())
    bridge = Bridge(str(random.randint(0, sys.maxsize)))
    result = bridge.create_virtual_env(num_ifaces)
    if result != testutils.SUCCESS:
        bridge.ns_del()
        testutils.log.error(
            "---------------------- Namespace creation failed ----------------------",
        )
        raise SystemExit("Unable to create the namespace environment.")
    testutils.log.info(
        "---------------------- Namespace successfully created ----------------------"
    )
    return bridge


def compile_program(options: Options, json_name: Path, info_name: Path) -> int:
    """Compile the input P4 program using p4c-bm2-ss."""
    testutils.log.info("---------------------- Compile with p4c-bm2-ss ----------------------")
    compilation_cmd = (
        f"{options.rootdir}/build/p4c-bm2-ss --target bmv2 --arch v1model "
        f"--p4runtime-files {info_name} {options.p4_file} -o {json_name}"
    )
    _, returncode = testutils.exec_process(compilation_cmd, timeout=30)
    if returncode != testutils.SUCCESS:
        testutils.log.error("Failed to compile the P4 program %s.", options.p4_file)
    return returncode


def get_iface_str(num_ifaces: int, prefix: str = "") -> str:
    """Produce the PTF interface arguments based on the number of interfaces the PTF test uses."""
    iface_str = ""
    for iface_num in range(num_ifaces):
        iface_str += f"-i {iface_num}@{prefix}{iface_num} "
    return iface_str


def run_simple_switch_grpc(
    options: Options, bridge: Bridge, switchlog: Path, grpc_port: int
) -> testutils.subprocess.Popen:
    """Start simple_switch_grpc and return the process handle."""
    thrift_port = testutils.pick_tcp_port(THRIFT_PORT)
    testutils.log.info(
        "---------------------- Start simple_switch_grpc ----------------------",
    )
    ifaces = get_iface_str(num_ifaces=options.num_ifaces)
    simple_switch_grpc = (
        f"simple_switch_grpc --thrift-port {thrift_port} --device-id 0 --log-file {switchlog} --log-flush "
        f"--packet-in ipc://{options.testdir}/bmv2_packets_1.ipc  --no-p4 "
        f"-- --grpc-server-addr 0.0.0.0:{grpc_port}")
    bridge_cmd = bridge.get_ns_prefix() + " " + simple_switch_grpc
    switch_proc = testutils.open_process(bridge_cmd)
    if switch_proc is None:
        bridge.ns_del()
        raise SystemExit("simple_switch_grpc ended with errors")

    return switch_proc


def run_ptf(
    options: Options, bridge: Bridge, grpc_port: int, json_name: Path, info_name: Path
) -> int:
    """Run the PTF test."""
    testutils.log.info("---------------------- Run PTF test ----------------------")
    # Add the file location to the python path.
    pypath = FILE_DIR
    # Show list of the tests
    testListCmd = f"ptf --pypath {pypath} --test-dir {options.testdir} --list"
    returncode = bridge.ns_exec(testListCmd)
    if returncode != testutils.SUCCESS:
        return returncode
    ifaces = get_iface_str(num_ifaces=options.num_ifaces, prefix="br_")
    test_params = f"grpcaddr='0.0.0.0:{grpc_port}';p4info='{info_name}';config='{json_name}';"
    tmp = "{0-8}"
    run_ptf_cmd = f"ptf --platform nn --device-socket 0-{tmp}@ipc://{options.testdir}/bmv2_packets_1.ipc --pypath {pypath}  --log-file {options.testdir.joinpath('ptf.log')}"
    run_ptf_cmd += f" --test-params={test_params} --test-dir {options.testdir}"
    returncode = bridge.ns_exec(run_ptf_cmd)
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
    grpc_port = testutils.pick_tcp_port(GRPC_PORT)
    switchlog = options.testdir.joinpath("switchlog")
    switch_proc = run_simple_switch_grpc(options, bridge, switchlog, grpc_port)
    # Run the PTF test and retrieve the result.
    result = run_ptf(options, bridge, grpc_port, json_name, info_name)
    if result != testutils.SUCCESS:
        # Terminate the switch process and emit its output in case of failure.
        testutils.kill_proc_group(switch_proc)
        if switchlog.with_suffix(".txt").exists():
            switchout = switchlog.with_suffix(".txt").read_text()
            testutils.log.error("######## Switch log ########\n%s", switchout)
        if switch_proc.stdout:
            out = switch_proc.stdout.read()
            # Do not bother to print whitespace.
            if out.strip():
                testutils.log.error("######## Switch output ######## \n%s", out)
        if switch_proc.stderr:
            err = switch_proc.stderr.read()
            # Do not bother to print whitespace.
            if err.strip():
                testutils.log.error("######## Switch errors ######## \n%s", err)
    bridge.ns_del()
    return result


def create_options(test_args) -> testutils.Optional[Options]:
    """Parse the input arguments and create a processed options object."""
    options = Options()
    options.p4_file = Path(testutils.check_if_file(test_args.p4_file))
    testfile = test_args.testfile
    if not testfile:
        testutils.log.info("No test file provided. Checking for file in folder.")
        testfile = options.p4_file.with_suffix(".py")
    result = testutils.check_if_file(testfile)
    if not result:
        return None
    options.testfile = Path(result)
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
        filename=options.testdir.joinpath("test.log"),
        format="%(levelname)s: %(message)s",
        level=getattr(logging, test_args.log_level),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    logging.getLogger().addHandler(stderr_log)
    return options


if __name__ == "__main__":
    if not testutils.check_root():
        testutils.log.error("This script requires root privileges; Exiting.")
        sys.exit(1)
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()

    test_options = create_options(args)
    if not test_options:
        sys.exit(testutils.FAILURE)
    # Run the test with the extracted options
    test_result = run_test(test_options)
    if not (args.nocleanup or test_result != testutils.SUCCESS):
        testutils.log.info("Removing temporary test directory.")
        testutils.del_dir(test_options.testdir)
    sys.exit(test_result)

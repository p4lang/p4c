#!/usr/bin/env python3

import argparse
import logging
import os
import random
import sys
import tempfile
import uuid
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
    "-nn",
    "--use-nanomsg",
    action="store_true",
    dest="use_nn",
    help="Use nanomsg for packet sending instead of virtual interfaces.",
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="WARNING",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)

GRPC_PORT: int = 28000
THRIFT_PORT: int = 22000
PTF_ADDR: str = "0.0.0.0"


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
    num_ifaces: int = 8
    # Whether to use nanomsg for packet delivery as opposed to Linux veth interfaces.
    use_nn: bool = False


class PTFTestEnv:
    options: Options = Options()
    switch_proc: testutils.subprocess.Popen = None

    def __init__(self, options):
        self.options = options

    def __del__(self):
        if self.switch_proc:
            # Terminate the switch process and emit its output in case of failure.
            testutils.kill_proc_group(self.switch_proc)

    def create_bridge(self, num_ifaces: int) -> Bridge:
        """Create a network namespace environment."""
        testutils.log.info(
            "---------------------- Creating a namespace ----------------------",
        )
        random.seed(datetime.now().timestamp())
        bridge = Bridge(uuid.uuid4())
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

    def compile_program(self, json_name: Path, info_name: Path) -> int:
        """Compile the input P4 program using p4c-bm2-ss."""
        testutils.log.info("---------------------- Compile with p4c-bm2-ss ----------------------")
        compilation_cmd = (
            f"{self.options.rootdir}/build/p4c-bm2-ss --target bmv2 --arch v1model "
            f"--p4runtime-files {info_name} {self.options.p4_file} -o {json_name}"
        )
        _, returncode = testutils.exec_process(compilation_cmd, timeout=30)
        if returncode != testutils.SUCCESS:
            testutils.log.error("Failed to compile the P4 program %s.", self.options.p4_file)
        return returncode

    def run_simple_switch_grpc(self, switchlog: Path, grpc_port: int) -> testutils.subprocess.Popen:
        raise NotImplementedError("method run_simple_switch_grpc not implemented for this class")

    def run_ptf(self, grpc_port: int, json_name: Path, info_name: Path) -> int:
        raise NotImplementedError("method run_ptf not implemented for this class")


class NNEnv(PTFTestEnv):
    bridge: Bridge = None

    def __init__(self, options):
        super().__init__(options)
        # Create the virtual environment for the test execution.
        self.bridge = self.create_bridge(options.num_ifaces)

    def __del__(self):
        if self.bridge:
            self.bridge.ns_del()
        super().__del__()

    def run_simple_switch_grpc(self, switchlog: Path, grpc_port: int) -> testutils.subprocess.Popen:
        """Start simple_switch_grpc and return the process handle."""
        testutils.log.info(
            "---------------------- Start simple_switch_grpc ----------------------",
        )
        thrift_port = testutils.pick_tcp_port(PTF_ADDR, THRIFT_PORT)
        simple_switch_grpc = (
            f"simple_switch_grpc --thrift-port {thrift_port} --device-id 0 --log-file {switchlog} --log-flush "
            f"--packet-in ipc://{self.options.testdir}/bmv2_packets_1.ipc  --no-p4 "
            f"-- --grpc-server-addr {PTF_ADDR}:{grpc_port} & "
        )
        bridge_cmd = self.bridge.get_ns_prefix() + " " + simple_switch_grpc
        self.switch_proc = testutils.open_process(bridge_cmd)
        return self.switch_proc

    def run_ptf(self, grpc_port: int, json_name: Path, info_name: Path) -> int:
        """Run the PTF test."""
        testutils.log.info("---------------------- Run PTF test ----------------------")
        # Add the file location to the python path.
        pypath = FILE_DIR
        # Show list of the tests
        testListCmd = f"ptf --pypath {pypath} --test-dir {self.options.testdir} --list"
        returncode = self.bridge.ns_exec(testListCmd)
        if returncode != testutils.SUCCESS:
            return returncode
        test_params = (
            f"grpcaddr='{PTF_ADDR}:{grpc_port}';p4info='{info_name}';config='{json_name}';"
            f"packet_wait_time='0.1';"
        )
        # TODO: There is currently a bug where we can not support more than 344 ports at once.
        # The nanomsg test back end simply hangs, the reason is unclear.
        port_range = "0-8"
        run_ptf_cmd = (
            f"ptf --platform nn --device-socket 0-{{{port_range}}}@ipc://{self.options.testdir}/"
            f"bmv2_packets_1.ipc --pypath {pypath} "
            f"--log-file {self.options.testdir.joinpath('ptf.log')} "
            f"--test-params={test_params} --test-dir {self.options.testdir}"
        )
        returncode = self.bridge.ns_exec(run_ptf_cmd)
        return returncode


class VethEnv(PTFTestEnv):
    bridge: Bridge = None

    def __init__(self, options):
        super().__init__(options)
        # Create the virtual environment for the test execution.
        self.bridge = self.create_bridge(options.num_ifaces)

    def __del__(self):
        if self.bridge:
            self.bridge.ns_del()
        super().__del__()

    def get_iface_str(self, num_ifaces: int, prefix: str = "") -> str:
        """Produce the PTF interface arguments based on the number of interfaces the PTF test uses."""
        iface_str = ""
        for iface_num in range(num_ifaces):
            iface_str += f"-i {iface_num}@{prefix}{iface_num} "
        return iface_str

    def run_simple_switch_grpc(self, switchlog: Path, grpc_port: int) -> testutils.subprocess.Popen:
        """Start simple_switch_grpc and return the process handle."""
        testutils.log.info(
            "---------------------- Start simple_switch_grpc ----------------------",
        )
        ifaces = self.get_iface_str(num_ifaces=self.options.num_ifaces)
        thrift_port = testutils.pick_tcp_port(PTF_ADDR, THRIFT_PORT)
        simple_switch_grpc = (
            f"simple_switch_grpc --thrift-port {thrift_port} --log-file {switchlog} --log-flush -i 0@0 "
            f"{ifaces} --no-p4 "
            f"-- --grpc-server-addr {PTF_ADDR}:{grpc_port}"
        )
        bridge_cmd = self.bridge.get_ns_prefix() + " " + simple_switch_grpc
        self.switch_proc = testutils.open_process(bridge_cmd)
        return self.switch_proc

    def run_ptf(self, grpc_port: int, json_name: Path, info_name: Path) -> int:
        """Run the PTF test."""
        testutils.log.info("---------------------- Run PTF test ----------------------")
        # Add the file location to the python path.
        pypath = FILE_DIR
        # Show list of the tests
        testListCmd = f"ptf --pypath {pypath} --test-dir {self.options.testdir} --list"
        returncode = self.bridge.ns_exec(testListCmd)
        if returncode != testutils.SUCCESS:
            return returncode
        ifaces = self.get_iface_str(num_ifaces=self.options.num_ifaces, prefix="br_")
        test_params = (
            f"grpcaddr='{PTF_ADDR}:{grpc_port}';p4info='{info_name}';config='{json_name}';"
            f"packet_wait_time='0.1';"
        )
        run_ptf_cmd = (
            f"ptf --pypath {pypath} {ifaces} --log-file {self.options.testdir.joinpath('ptf.log')} "
            f"--test-params={test_params} --test-dir {self.options.testdir}"
        )
        returncode = self.bridge.ns_exec(run_ptf_cmd)
        return returncode


def run_test(options: Options) -> int:
    """Define the test environment and compile the P4 target
    Optional: Run the generated model"""
    test_name = Path(options.p4_file.name)
    json_name = options.testdir.joinpath(test_name.with_suffix(".json"))
    info_name = options.testdir.joinpath(test_name.with_suffix(".p4info.txt"))
    # Copy the test file into the test folder so that it can be picked up by PTF.
    testutils.copy_file(options.testfile, options.testdir)

    if options.use_nn:
        testenv: PTFTestEnv = NNEnv(options)
    else:
        testenv = VethEnv(options)

    # Compile the P4 program.
    returncode = testenv.compile_program(json_name, info_name)
    if returncode != testutils.SUCCESS:
        return returncode

    # Pick available ports for the gRPC switch.
    switchlog = options.testdir.joinpath("switchlog")
    grpc_port = testutils.pick_tcp_port(PTF_ADDR, GRPC_PORT)
    switch_proc = testenv.run_simple_switch_grpc(switchlog, grpc_port)
    if switch_proc is None:
        return testutils.FAILURE
    # Run the PTF test and retrieve the result.
    result = testenv.run_ptf(grpc_port, json_name, info_name)
    # Delete the test environment and trigger a clean up.
    del testenv
    # Print switch log if the results were not successful.
    if result != testutils.SUCCESS:
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

    try:
        import nnpy  # pylint: disable=W0611,C0415

        assert nnpy
        options.use_nn = args.use_nn
    except ImportError:
        testutils.log.error("nnpy is not available on this system. Falling back to veth testing.")
        options.use_nn = False

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
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()

    test_options = create_options(args)
    if not test_options:
        sys.exit(testutils.FAILURE)

    if not testutils.check_root():
        testutils.log.error("This script requires root privileges; Exiting.")
        sys.exit(1)

    # Run the test with the extracted options
    test_result = run_test(test_options)
    if not (args.nocleanup or test_result != testutils.SUCCESS):
        testutils.log.info("Removing temporary test directory.")
        testutils.del_dir(test_options.testdir)
    sys.exit(test_result)

#!/usr/bin/env python3

import argparse
import logging
import os
import random
import sys
import tempfile
import time
import uuid
from datetime import datetime
from pathlib import Path

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
TOOLS_PATH = FILE_DIR.joinpath("../../tools")
sys.path.append(str(TOOLS_PATH))
import testutils

BRIDGE_PATH = FILE_DIR.joinpath("../ebpf/targets")
sys.path.append(str(BRIDGE_PATH))
from ebpfenv import Bridge

PARSER = argparse.ArgumentParser()
PARSER.add_argument("p4c_dir", help="The location of the the compiler source directory")
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
    "--ipdk-install-dir",
    dest="ipdk_install_dir",
    required=True,
    help="The location of the IPDK installation folder for infrap4d-related executables.",
)
PARSER.add_argument(
    "--ld-library-path",
    dest="ld_library_path",
    type=str,
    help="The location of the shared libs for ipdk and dpdk. "
    "By default, these libraries are located in `ipdk_install_dir`.",
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
    "--num-taps",
    default=2,
    dest="num_taps",
    help="How many TAPs to create.",
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="WARNING",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)

# 9559 is the default P4Runtime API server port
GRPC_PORT: int = 9559
PTF_ADDR: str = "0.0.0.0"


# Check if target ports are ready to be connected (make sure infrap4d is on)
def is_port_alive(ns, port) -> bool:
    command = f"sudo ip netns exec {ns} netstat -tuln"
    out, _ = testutils.exec_process(command, timeout=10, capture_output=True)
    if str(port) in out:
        return True
    return False


class Options:
    """Options for this testing script. Usually correspond to command line inputs."""

    # File that is being compiled.
    p4_file: Path = Path(".")
    # Path to ptf test file that is used.
    testfile: Path = Path(".")
    # Actual location of the test framework.
    testdir: Path = Path(".")
    # The base directory where tests are executed.
    p4c_dir: Path = Path(".")
    # Folder containing the IPDK binaries.
    ipdk_install_dir: Path = Path(".")
    # LD_LIBRARY_PATH.
    ld_library_path: str = ""
    # Number of TAPs to create.
    num_taps: int = 2


class PTFTestEnv:
    options: Options = Options()
    switch_proc: testutils.subprocess.Popen = None
    proc_env_vars: dict = {}

    def __init__(self, options):
        self.options = options
        # Create the virtual environment for the test execution.
        self.bridge = self.create_bridge()

    def __del__(self):
        if self.switch_proc:
            # Terminate the switch process and emit its output in case of failure.
            testutils.kill_proc_group(self.switch_proc)
        if self.bridge:
            self.bridge.ns_del()

    def create_bridge(self) -> Bridge:
        """Create a network namespace environment with Bridge."""
        testutils.log.info(
            "---------------------- Creating a namespace ----------------------",
        )
        random.seed(datetime.now().timestamp())
        bridge = Bridge(uuid.uuid4())
        result = bridge.create_virtual_env(0)
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

    def create_TAPs(self, proc_env_vars: dict, insecure_mode: bool = True) -> int:
        """Create TAPs with gNMI"""
        testutils.log.info(
            "---------------------- Creating TAPs ----------------------",
        )
        for index in range(self.options.num_taps):
            tap_name = f"TAP{index}"
            cmd = (
                f"{self.options.ipdk_install_dir}/bin/gnmi-ctl set "
                f"device:virtual-device,name:{tap_name}"
                f",pipeline-name:pipe,mempool-name:MEMPOOL0,mtu:1500,port-type:TAP "
                f"-grpc_use_insecure_mode={insecure_mode}"
            )
            returncode = self.bridge.ns_exec(cmd, env=proc_env_vars)
            if returncode != testutils.SUCCESS:
                testutils.log.error("Failed to create TAP")
                return returncode
            returncode = self.bridge.ns_exec(f"ifconfig {tap_name} up")
            if returncode != testutils.SUCCESS:
                testutils.log.error("Failed to activate TAP interface")
                return returncode
        return returncode

    def compile_program(
        self, info_name: Path, bf_rt_schema: Path, context: Path, dpdk_spec: Path
    ) -> int:
        # Create /pipe directory
        _, returncode = testutils.exec_process(
            f"mkdir {self.options.testdir.joinpath('pipe')}", timeout=30
        )
        if returncode != testutils.SUCCESS:
            testutils.log.error("Failed to create /pipe directory")
            return returncode

        # Compile the input P4 program using p4c-dpdk.
        testutils.log.info("---------------------- Compile with p4c-dpdk ----------------------")
        compilation_cmd = f"{self.options.p4c_dir}/build/p4c-dpdk --arch pna --target dpdk \
            --p4runtime-files {info_name} \
            --bf-rt-schema {bf_rt_schema} \
            --context {context} \
            --tdi-builder-conf {self.options.testdir.joinpath(Path(self.options.p4_file.name).with_suffix('.conf'))}\
            -o {dpdk_spec} {self.options.p4_file}"
        _, returncode = testutils.exec_process(compilation_cmd, timeout=30)
        if returncode != testutils.SUCCESS:
            testutils.log.error("Failed to compile the P4 program %s.", self.options.p4_file)
        return returncode

    def run_infrap4d(
        self, proc_env_vars: dict, options: Options, insecure_mode: bool = True
    ) -> testutils.subprocess.Popen:
        # Start infrap4d and return the process handle.
        testutils.log.info(
            "---------------------- Start infrap4d ----------------------",
        )
        log_dir = options.testdir.joinpath("infrap4d")
        testutils.check_and_create_dir(log_dir)

        run_infrap4d_cmd = (
            f"{self.options.ipdk_install_dir}/sbin/infrap4d "
            f"-grpc_open_insecure_mode={insecure_mode} "
            f"-log_dir={log_dir} "
            f"-dpdk_sde_install={options.ipdk_install_dir} "
            f"-dpdk_infrap4d_cfg={options.ipdk_install_dir}/share/stratum/dpdk/dpdk_skip_p4.conf "
            f"-chassis_config_file={options.ipdk_install_dir}/share/stratum/dpdk/dpdk_port_config.pb.txt "
        )
        bridge_cmd = self.bridge.get_ns_prefix() + " " + run_infrap4d_cmd
        self.switch_proc = testutils.open_process(bridge_cmd, env=proc_env_vars)
        cnt = 1
        while not is_port_alive(self.bridge.ns_name, GRPC_PORT) and cnt != 5:
            time.sleep(2)
            cnt += 1
            testutils.log.info("Cannot connect to Infrap4d: " + str(cnt) + " try")
        if not is_port_alive(self.bridge.ns_name, GRPC_PORT):
            return testutils.FAILURE
        return self.switch_proc

    def build_and_load_pipeline(
        self, p4c_conf: Path, conf_bin: Path, info_name: Path, proc_env_vars: dict
    ) -> int:
        testutils.log.info("---------------------- Build and Load Pipeline ----------------------")
        command = (
            f"{self.options.ipdk_install_dir}/bin/tdi_pipeline_builder "
            f"--p4c_conf_file={p4c_conf} "
            f"--bf_pipeline_config_binary_file={conf_bin}"
        )

        _, returncode = testutils.exec_process(command, timeout=30, env=proc_env_vars)
        if returncode != testutils.SUCCESS:
            testutils.log.error("Failed to build pipeline")
            return returncode

        # Load pipeline.
        command = (
            f"{self.options.ipdk_install_dir}/bin/p4rt-ctl "
            "set-pipe br0 "
            f"{conf_bin} "
            f"{info_name} "
        )
        returncode = self.bridge.ns_exec(command, timeout=30)
        if returncode != testutils.SUCCESS:
            testutils.log.error("Failed to load pipeline")
            return returncode
        return testutils.SUCCESS

    def run_ptf(self, grpc_port: int) -> int:
        """Run the PTF test."""
        testutils.log.info("---------------------- Run PTF test ----------------------")
        # Add the file location to the python path.
        pypath = FILE_DIR
        # Show list of the tests
        testListCmd = f"ptf --pypath {pypath} --test-dir {self.options.testdir} --list"
        returncode = self.bridge.ns_exec(testListCmd)
        if returncode != testutils.SUCCESS:
            return returncode
        taps: str = ""
        for index in range(self.options.num_taps):
            taps += f" -i {index}@TAP{index}"
        test_params = f"grpcaddr='{PTF_ADDR}:{grpc_port}'"
        run_ptf_cmd = (
            f"ptf --pypath {pypath} {taps} --log-file {self.options.testdir.joinpath('ptf.log')} "
            f"--test-params={test_params} --test-dir {self.options.testdir}"
        )
        returncode = self.bridge.ns_exec(run_ptf_cmd)
        return returncode


def run_test(options: Options) -> int:
    # Add necessary environment variables for libs and executables
    proc_env_vars: dict = os.environ.copy()
    if "LD_LIBRARY_PATH" in proc_env_vars:
        proc_env_vars["LD_LIBRARY_PATH"] += f"{options.ld_library_path}"
    else:
        proc_env_vars["LD_LIBRARY_PATH"] = f"{options.ld_library_path}"
    if "PATH" in proc_env_vars:
        proc_env_vars["PATH"] += f"{options.ipdk_install_dir}/bin"
    else:
        proc_env_vars["PATH"] = f"{options.ipdk_install_dir}/bin"
    proc_env_vars["PATH"] += f"{options.ipdk_install_dir}/sbin"
    proc_env_vars["SDE_INSTALL"] = f"{options.ipdk_install_dir}"

    # Define the test environment and compile the P4 target
    test_name = Path(options.p4_file.name)
    info_name = options.testdir.joinpath("p4Info.txt")
    bf_rt_schema = options.testdir.joinpath("bf-rt.json")
    conf_bin = options.testdir.joinpath(test_name.with_suffix(".pb.bin"))
    # Files needed by the pipeline
    context = options.testdir.joinpath("pipe/context.json")
    p4c_conf = options.testdir.joinpath(test_name.with_suffix(".conf"))
    dpdk_spec = options.testdir.joinpath(f"pipe/{test_name.with_suffix('.spec')}")

    # Copy the test file into the test folder so that it can be picked up by PTF.
    testutils.copy_file(options.testfile, options.testdir)

    # Create the test environment
    testenv = PTFTestEnv(options)

    # Compile the P4 program.
    returncode = testenv.compile_program(info_name, bf_rt_schema, context, dpdk_spec)
    if returncode != testutils.SUCCESS:
        return returncode

    # Run the switch.
    switch_proc = testenv.run_infrap4d(proc_env_vars, options)
    if switch_proc is None:
        return testutils.FAILURE

    # Create the TAP interfaces.
    returncode = testenv.create_TAPs(proc_env_vars)
    if returncode != testutils.SUCCESS:
        return returncode

    # Build and load the pipeline
    returncode = testenv.build_and_load_pipeline(p4c_conf, conf_bin, info_name, proc_env_vars)
    if returncode != testutils.SUCCESS:
        return returncode

    # Run the PTF test and retrieve the result.
    result = testenv.run_ptf(GRPC_PORT)
    # Delete the test environment and trigger a clean up.
    del testenv
    # Print switch log if the results were not successful.
    if result != testutils.SUCCESS:
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
    options.p4c_dir = Path(test_args.p4c_dir)
    options.ipdk_install_dir = Path(test_args.ipdk_install_dir)
    if test_args.ld_library_path:
        options.ld_library_path = test_args.ld_library_path
    else:
        options.ld_library_path = (
            f"{options.ipdk_install_dir}/lib;{options.ipdk_install_dir}/lib/x86_64-linux-gnu"
        )
    options.num_taps = test_args.num_taps

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

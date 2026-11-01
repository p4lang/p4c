#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import uuid
from dataclasses import dataclass
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
    "--compiler", type=Path, help="Path to p4c-dpdk (defaults to p4c_dir/build/p4c-dpdk)."
)
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
    help="Keep temporary results even when the test succeeds.",
)
PARSER.add_argument(
    "-n",
    "--num-taps",
    default=2,
    type=int,
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
P4RUNTIME_PORT: int = 9559
GNMI_PORT: int = 9339
PTF_ADDR: str = "0.0.0.0"


def check_python_module(module: str, proc_env_vars: dict) -> bool:
    """Return True if module can be imported by the current Python."""
    result = subprocess.run(
        [sys.executable, "-c", f"import {module}"],
        env=proc_env_vars,
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode == testutils.SUCCESS


def get_ptf_runner(proc_env_vars: dict) -> testutils.Optional[str]:
    """Return an executable path for the ptf runner."""
    venv_ptf = Path(sys.executable).with_name("ptf")
    if venv_ptf.exists() and os.access(venv_ptf, os.X_OK):
        return str(venv_ptf)
    return shutil.which("ptf", path=proc_env_vars.get("PATH"))


# Check if target ports are ready to be connected (make sure infrap4d is on)
def is_port_alive(ns, port) -> bool:
    result = subprocess.run(
        ["ip", "netns", "exec", str(ns), "ss", "-H", "-ltn", f"sport = :{port}"],
        timeout=10,
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode == 0 and bool(result.stdout.strip())


@dataclass
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
    compiler: Path = Path("p4c-dpdk")
    # Folder containing the IPDK binaries.
    ipdk_install_dir: Path = Path(".")
    # LD_LIBRARY_PATH.
    ld_library_path: str = ""
    # Number of TAPs to create.
    num_taps: int = 2
    # Executable path for ptf runner.
    ptf_runner: str = "ptf"


def normalize_library_path(path_value: str) -> str:
    """Normalize user-provided library path separators to ':' for Linux."""
    return path_value.replace(";", ":")


def default_library_path(ipdk_install_dir: Path) -> str:
    """Build a sane default LD_LIBRARY_PATH for IPDK/Stratum runtime tools."""
    candidate_dirs = [
        ipdk_install_dir / "lib",
        ipdk_install_dir / "lib/x86_64-linux-gnu",
    ]

    # Support both common dependency env names.
    for deps_env in ("DEPEND_INSTALL", "DEPS_INSTALL_DIR"):
        deps_dir = os.environ.get(deps_env)
        if deps_dir:
            deps_path = Path(deps_dir)
            candidate_dirs.extend([deps_path / "lib", deps_path / "lib/x86_64-linux-gnu"])

    existing_dirs = [str(lib_dir) for lib_dir in candidate_dirs if lib_dir.exists()]
    return ":".join(existing_dirs)


class PTFTestEnv:
    def __init__(self, options):
        self.options = options
        self.switch_proc = None
        self.switch_log = None
        self.bridge = None
        # Create the virtual environment for the test execution.
        self.bridge = self.create_bridge()

    def close(self):
        """Reap infrap4d before removing its namespace, including on setup failures."""
        try:
            if self.switch_proc is not None:
                if self.switch_proc.poll() is None:
                    try:
                        testutils.kill_proc_group(self.switch_proc)
                    except ProcessLookupError:
                        # The target may have exited between poll() and killpg().
                        pass
                try:
                    self.switch_proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(self.switch_proc.pid, signal.SIGKILL)
                    self.switch_proc.wait()
        finally:
            if self.switch_log is not None:
                self.switch_log.close()
            if self.bridge is not None:
                self.bridge.ns_del()

    def log_switch_output(self):
        log_dir = self.options.testdir / "infrap4d"
        for name in ("output.log", "infrap4d.ERROR", "infrap4d.WARNING", "infrap4d.INFO"):
            path = log_dir / name
            if path.exists():
                testutils.log.error("######## %s ########\n%s", name, path.read_text())

    def create_bridge(self) -> Bridge:
        """Create a network namespace environment with Bridge."""
        testutils.log.info(
            "---------------------- Creating a namespace ----------------------",
        )
        bridge = Bridge(str(uuid.uuid4()))
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

    def create_taps(self, proc_env_vars: dict, insecure_mode: bool = True) -> int:
        """Create TAPs with gNMI"""
        testutils.log.info(
            "---------------------- Creating TAPs ----------------------",
        )
        returncode = testutils.SUCCESS
        for index in range(self.options.num_taps):
            tap_name = f"TAP{index}"
            cmd = (
                f"{self.options.ipdk_install_dir}/bin/gnmi-ctl set "
                f"device:virtual-device,name:{tap_name}"
                f",pipeline-name:pipe,mempool-name:MEMPOOL0,mtu:1500,port-type:TAP "
                f"-grpc_addr={PTF_ADDR}:{GNMI_PORT} "
                f"-grpc_use_insecure_mode={insecure_mode}"
            )
            returncode = self.bridge.ns_exec(cmd, env=proc_env_vars)
            if returncode != testutils.SUCCESS:
                testutils.log.error("Failed to create TAP")
                return returncode
            returncode = self.bridge.ns_exec(f"ip link set dev {tap_name} up")
            if returncode != testutils.SUCCESS:
                testutils.log.error("Failed to activate TAP interface")
                return returncode
        return returncode

    def compile_program(
        self, info_name: Path, bf_rt_schema: Path, context: Path, dpdk_spec: Path
    ) -> int:
        self.options.testdir.joinpath("pipe").mkdir(exist_ok=True)

        # Compile the input P4 program using p4c-dpdk.
        testutils.log.info("---------------------- Compile with p4c-dpdk ----------------------")
        compilation_cmd = f"{self.options.compiler} --arch pna --target dpdk \
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
        self, proc_env_vars: dict, insecure_mode: bool = True
    ) -> testutils.Optional[testutils.subprocess.Popen]:
        # Start infrap4d and return the process handle.
        testutils.log.info(
            "---------------------- Start infrap4d ----------------------",
        )
        log_dir = self.options.testdir.joinpath("infrap4d")
        testutils.check_and_create_dir(log_dir)

        run_infrap4d_cmd = (
            f"{self.options.ipdk_install_dir}/sbin/infrap4d "
            f"-grpc_open_insecure_mode={insecure_mode} "
            f"-log_dir={log_dir} "
            f"-detach=false "
            # P4 programs without tables/actions are valid, including parser-only tests.
            f"-skip_p4_min_objects_check=true "
            f"-external_stratum_urls={PTF_ADDR}:{P4RUNTIME_PORT},{PTF_ADDR}:{GNMI_PORT} "
            f"-dpdk_sde_install={self.options.ipdk_install_dir} "
            f"-dpdk_infrap4d_cfg={self.options.ipdk_install_dir}/share/stratum/dpdk/dpdk_skip_p4.conf "
            f"-chassis_config_file={self.options.ipdk_install_dir}/share/stratum/dpdk/dpdk_port_config.pb.txt "
        )
        bridge_cmd = self.bridge.get_ns_prefix() + " " + run_infrap4d_cmd
        # Do not leave PIPEs undrained while PTF runs: verbose target logs can fill them.
        self.switch_log = (log_dir / "output.log").open("w")
        self.switch_proc = testutils.open_process(
            bridge_cmd, env=proc_env_vars, stdout=self.switch_log, stderr=subprocess.STDOUT
        )
        for _ in range(10):
            if self.switch_proc is None or self.switch_proc.poll() is not None:
                break
            if all(
                is_port_alive(self.bridge.ns_name, port) for port in (P4RUNTIME_PORT, GNMI_PORT)
            ):
                return self.switch_proc
            time.sleep(2)
        testutils.log.error("Infrap4d did not start its P4Runtime and gNMI servers")
        return None

    def build_pipeline(self, p4c_conf: Path, conf_bin: Path, proc_env_vars: dict) -> int:
        testutils.log.info("---------------------- Build Pipeline ----------------------")
        builder = f"{self.options.ipdk_install_dir}/bin/tdi_pipeline_builder"

        command = [
            builder,
            f"--p4c_conf_file={p4c_conf}",
            f"--tdi_pipeline_config_binary_file={conf_bin}",
        ]
        testutils.log.info("Executing command: %s", " ".join(command))
        result = subprocess.run(
            command,
            env=proc_env_vars,
            capture_output=True,
            text=True,
            check=False,
            timeout=30,
        )
        if result.stdout:
            testutils.log.info(result.stdout.rstrip())
        if result.stderr:
            testutils.log.warning(result.stderr.rstrip())
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to build pipeline")
            return result.returncode
        if not conf_bin.exists():
            testutils.log.error("Expected pipeline config was not generated: %s", conf_bin)
            return testutils.FAILURE
        return testutils.SUCCESS

    def run_ptf(self, P4RUNTIME_PORT: int, info_name, conf_bin) -> int:
        """Run the PTF test."""
        testutils.log.info("---------------------- Run PTF test ----------------------")
        # Add the tools PTF folder to the python path, it contains the base test.
        pypath = TOOLS_PATH.joinpath("ptf")
        # Show list of the tests
        testListCmd = (
            f"{self.options.ptf_runner} --pypath {pypath} --test-dir {self.options.testdir} --list"
        )
        returncode = self.bridge.ns_exec(testListCmd)
        if returncode != testutils.SUCCESS:
            return returncode
        taps: str = ""
        for index in range(self.options.num_taps):
            taps += f" -i {index}@TAP{index}"
        test_params = (
            f"grpcaddr='{PTF_ADDR}:{P4RUNTIME_PORT}';p4info='{info_name}';config='{conf_bin}';"
        )
        test_params += "device_id=1"
        run_ptf_cmd = (
            f"{self.options.ptf_runner} --pypath {pypath} {taps} "
            f"--log-file {self.options.testdir.joinpath('ptf.log')} "
            f"--test-params={test_params} --test-dir {self.options.testdir}"
        )
        returncode = self.bridge.ns_exec(run_ptf_cmd)
        return returncode


def run_test(options: Options) -> int:
    # Add necessary environment variables for libs and executables
    proc_env_vars: dict = os.environ.copy()
    existing_ld_library_path = proc_env_vars.get("LD_LIBRARY_PATH", "")
    proc_env_vars["LD_LIBRARY_PATH"] = ":".join(
        [value for value in [options.ld_library_path, existing_ld_library_path] if value]
    )
    proc_env_vars["SDE_INSTALL"] = f"{options.ipdk_install_dir}"

    if not check_python_module("ptf", proc_env_vars):
        testutils.log.error(
            "The Python module 'ptf' is not available for %s. "
            "Run this test via `uv run` (recommended) or install the module in the active Python.",
            sys.executable,
        )
        return testutils.FAILURE

    ptf_runner = get_ptf_runner(proc_env_vars)
    if ptf_runner is None:
        testutils.log.error(
            "Unable to locate the 'ptf' executable. "
            "Run this test via `uv run` (recommended) or ensure 'ptf' is on PATH."
        )
        return testutils.FAILURE
    options.ptf_runner = ptf_runner

    # Define the test environment and compile the P4 target
    test_name = Path(options.p4_file.name)
    info_name = options.testdir.joinpath("p4Info.txtpb")
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

    result = testutils.FAILURE
    try:
        returncode = testenv.compile_program(info_name, bf_rt_schema, context, dpdk_spec)
        if returncode != testutils.SUCCESS:
            return returncode
        if not p4c_conf.exists():
            testutils.log.error("Expected p4c conf was not generated: %s", p4c_conf)
            return testutils.FAILURE

        returncode = testenv.build_pipeline(p4c_conf, conf_bin, proc_env_vars)
        if returncode != testutils.SUCCESS:
            return returncode
        if testenv.run_infrap4d(proc_env_vars) is None:
            return testutils.FAILURE
        returncode = testenv.create_taps(proc_env_vars)
        if returncode != testutils.SUCCESS:
            return returncode

        # The PTF base test loads the pipeline through P4Runtime.
        result = testenv.run_ptf(P4RUNTIME_PORT, info_name, conf_bin)
        return result
    except (OSError, subprocess.TimeoutExpired) as error:
        testutils.log.error("Unable to run DPDK PTF test: %s", error)
        return testutils.FAILURE
    finally:
        testenv.close()
        if result != testutils.SUCCESS:
            testenv.log_switch_output()


def create_options(test_args) -> testutils.Optional[Options]:
    """Parse the input arguments and create a processed options object."""
    options = Options()
    p4_file = testutils.check_if_file(test_args.p4_file)
    if not p4_file:
        return None
    options.p4_file = Path(p4_file)
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
    options.testdir = Path(testdir).resolve()
    options.testdir.mkdir(parents=True, exist_ok=True)
    options.p4c_dir = Path(test_args.p4c_dir).resolve()
    options.compiler = (test_args.compiler or options.p4c_dir / "build/p4c-dpdk").resolve()
    options.ipdk_install_dir = Path(test_args.ipdk_install_dir).resolve()
    if test_args.ld_library_path:
        options.ld_library_path = normalize_library_path(test_args.ld_library_path)
    else:
        options.ld_library_path = default_library_path(options.ipdk_install_dir)
    options.num_taps = test_args.num_taps
    if options.num_taps < 1:
        testutils.log.error("--num-taps must be positive")
        return None

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
    args = PARSER.parse_args()

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

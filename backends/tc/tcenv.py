#!/usr/bin/env python3
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
""" Virtual environment which models a simple bridge with n attached
    interfaces. The bridge runs in a completely isolated namespace.
    Allows the loading and testing of eBPF programs. """

import logging
import os
import re
import socket
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, List, Optional

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../..")))
from tools import testutils  # pylint: disable=wrong-import-position


class NS:
    def __init__(self, namespace: str, runtime_dir: str, virtme):
        # Identifier of the namespace.
        self.ns_name: str = namespace
        # List of the veth pair bridge ports.
        self.port0 = 'p4port0'
        self.port0_pair = 'port0'
        self.port1 = 'p4port1'
        self.port1_pair = 'port1'
        self.block_num = '21'
        self.runtime_dir = runtime_dir
        self.mnt_dir = '/mnt'
        self.port_mapping = (self.port0_pair, self.port1_pair)
        self.virtme = virtme

    def ns_init_cmd(self) -> int:
        """Initialize the namespace."""
        return f"{self.mnt_dir}/build-simple-p4 {self.ns_name} {self.port0} {self.port0_pair} {self.port1} {self.port1_pair} {self.block_num}"

    def ns_del_cmd(self) -> int:
        """Initialize the namespace."""

        return f"ip netns del {self.ns_name}"

    def ns_init(self) -> int:
        """Initialize the namespace."""
        cmd = f"{self.mnt_dir}/build-simple-p4 {self.ns_name} {self.port0} {self.port0_pair} {self.port1} {self.port1_pair} {self.block_num}"
        result = testutils.exec_process(cmd)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to create namespace %s", self.ns_name)

        return result.returncode

    def ns_del(self) -> int:
        """Delete the namespace and with it all the process running in it."""
        cmd = f"ip netns del {self.ns_name}"
        result = testutils.exec_process(cmd, shell=True)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to delete namespace %s", self.ns_name)
        return result.returncode

    def get_ns_prefix(self) -> str:
        """Return the command prefix for the namespace of this bridge class."""
        return f'ip netns exec {self.ns_name}'

    def get_intros_prefix(self) -> str:
        """Return the command prefix for the namespace of this bridge class."""
        return "EXPORT INTROSPECTION=.;"

    def _ns_exec(self, cmd: str, **extra_args: Any):
        # bash -c allows us to run multiple commands at once
        result = testutils.exec_process(cmd, **extra_args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to run command in namespace %s", self.ns_name)
        return result.returncode

    def ns_exec(self, cmd_string: str, **extra_args: Any) -> int:
        """Run and execute an isolated command in the namespace."""
        cmd = self.get_ns_prefix() + " " + cmd_string
        return self._ns_exec(cmd, **extra_args)

    def ns_exec_intros(self, cmd_string: str, **extra_args: Any) -> int:
        """Run and execute an isolated command in the namespace."""
        cmd = f"{self.get_ns_prefix()} {self.get_intros_prefix()} {cmd_string}"
        return self._ns_exec(cmd, **extra_args)

    def ns_proc_open(self) -> Optional[subprocess.Popen]:
        """Open a bash process in the namespace and return the handle"""
        cmd = self.get_ns_prefix() + " bash"
        return testutils.open_process(cmd)


class Virtme:
    def __init__(
        self,
        namespace,
        runtime_dir,
        iproute2_dir,
        spawn_script,
        post_boot_script,
        tmpdir_basename,
        verbose,
    ):
        self.ns = NS(namespace, runtime_dir, self)
        self.runtime_dir = runtime_dir
        self.iproute2_dir = f"{self.ns.mnt_dir}/{iproute2_dir}"
        self.vm_proc = None
        self.extract_dir = f"{runtime_dir}/extracted_deb"
        self.post_boot_script = post_boot_script
        self.spawn_script = spawn_script
        self.host_ip = "10.10.11.7"
        self.tmpdir_basename = tmpdir_basename
        self.release_url = 'https://api.github.com/repos/p4tc-dev/linux-p4tc-pub/releases/latest'
        self.port_mapping = (self.ns.port0, self.ns.port1)
        self.verbose = verbose
        self.user = "ci"

    def __del__(self):
        if self.vm_proc:
            testutils.kill_proc_group(self.vm_proc)

    def _get_version(self, url):
        pattern = r"(\d+\.\d+\.0p\d+tc-v\d+-rc\d+)"

        match = re.search(pattern, url)

        if match:
            version = match.group(1)
            if self.verbose:
                print(f"Extracted version: {version}")
        else:
            testutils.log.error("Version string not found")
            return None

        return version + "+"

    def extract_version(self):
        cmd = f'curl -s {self.release_url} | jq .assets[2].browser_download_url | tr -d \\"'
        result = testutils.exec_process(cmd, shell=True)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to download deb package")
            return None

        image_url = result.output

        return self._get_version(image_url)

    def wait_for_ssh(self):
        # Wait for 30 seconds at most.
        max_tries = 6
        tries = 0
        while True:
            try:
                with socket.create_connection((self.host_ip, 22), timeout=1):
                    if self.verbose:
                        print("Connection established")
                    return
            except (socket.timeout, ConnectionRefusedError, OSError):
                if self.verbose:
                    print("Connection still refused")
                time.sleep(5)
                tries += 1

    def boot(self):
        version = self.extract_version()
        if version is None:
            return testutils.FAILURE
        config_str = f"{self.extract_dir}/boot/config-{version}"
        img_str = f"{self.extract_dir}/boot/vmlinuz-{version}"

        cmd = f"{self.spawn_script} -f {config_str} -i {img_str} -m {self.runtime_dir} -s {self.post_boot_script} -n {self.host_ip} -u {self.user}"
        self.vm_proc = testutils.open_process(cmd, env=os.environ.copy())
        if self.vm_proc is None:
            testutils.log.error("Failed to boot vm")
            return testutils.FAILURE

        # self.monitor_process(self.vm_proc)
        self.wait_for_ssh()

        return testutils.SUCCESS

    def get_virtme_prefix(self):
        return f'ssh {self.user}@{self.host_ip} -q -i {self.runtime_dir}/id_rsa -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null sudo'

    def ns_init(self):
        cmd = f'{self.get_virtme_prefix()} {self.ns.ns_init_cmd()}'
        result = testutils.exec_process(cmd)
        return result.returncode

    def ns_del(self):
        cmd = f'{self.get_virtme_prefix()} {self.ns.ns_del_cmd()}'
        return testutils.exec_process(cmd)

    def run_intros(self, cmd):
        intros_dir = f"{self.ns.mnt_dir}/{self.tmpdir_basename}"
        tc_path = f"{self.iproute2_dir}/tc/tc"
        cmd = cmd.format(tc=tc_path, introspection=intros_dir)
        cmd = f'{self.get_virtme_prefix()} {self.ns.get_ns_prefix()} /bin/bash -c \"export INTROSPECTION={intros_dir}; export TC={tc_path}; {cmd}\"'
        result = testutils.exec_process(cmd)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to run command with introspection")

        return result.returncode

    def run(self, cmd):
        result = testutils.exec_process(f"{self.get_virtme_prefix()} {cmd}")
        return result.returncode

    def run_script(self, script_name, args):
        cmd = f'sudo -E {self.ns.mnt_dir}/{script_name} ' + " ".join(args)
        return self.run(cmd)

    def run_tcpdump(self, filename, port):
        remote_pid = -1
        # Define the SSH command
        tcpdump_cmd = f"tcpdump -w {filename} -i {port}"
        ssh_command = [
            "ssh",
            f"{self.user}@{self.host_ip}",
            "-i",
            f"{self.runtime_dir}/id_rsa",
            "-q",
            "-o",
            "StrictHostKeyChecking=no",
            "-o",
            "UserKnownHostsFile=/dev/null",
            f"sudo -E nohup bash -c '{tcpdump_cmd} > /dev/null 2>&1 &' && sleep 1 && pgrep -n -f '{tcpdump_cmd}'",
        ]

        # Run the command using subprocess to capture the output
        process = subprocess.Popen(ssh_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # Read the output (the PID)
        stdout, stderr = process.communicate()

        # Check if we got a PID back
        if process.returncode == 0:
            remote_pid = stdout.strip().decode()

        return remote_pid


def main() -> None:
    """main"""
    # This is a simple test script which creates/deletes a virtual environment

    # Configure logging.
    logging.basicConfig(
        filename="ebpfenv.log",
        format="%(levelname)s:%(message)s",
        level=logging.INFO,
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s:%(message)s"))
    logging.getLogger().addHandler(stderr_log)


if __name__ == "__main__":
    main()

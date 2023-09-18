#!/usr/bin/env python3
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
""" Virtual environment which models a simple bridge with n attached
    interfaces. The bridge runs in a completely isolated namespace.
    Allows the loading and testing of eBPF programs. """

import logging
import sys
from pathlib import Path
from typing import List

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../../tools")))
import testutils


class Bridge:
    def __init__(self, namespace: str):
        # Identifier of the namespace.
        self.ns_name: str = namespace
        # Name of the central bridge.
        self.br_name: str = "core"
        # List of the veth pair bridge ports.
        self.br_ports: List[str] = []
        # List of the veth pair edge ports.
        self.edge_ports: List[str] = []

    def ns_init(self) -> int:
        """Initialize the namespace."""
        cmd = f"ip netns add {self.ns_name}"
        result = testutils.exec_process(cmd)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to create namespace %s", self.ns_name)
            return result.returncode
        return self.ns_exec("ip link set dev lo up")

    def ns_del(self) -> int:
        """Delete the namespace and with it all the process running in it."""
        cmd = f"ip netns pids {self.ns_name} | xargs -r kill; ip netns del {self.ns_name}"
        result = testutils.exec_process(cmd, shell=True)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to delete namespace %s", self.ns_name)
        return result.returncode

    def get_ns_prefix(self) -> str:
        """Return the command prefix for the namespace of this bridge class."""
        return f"ip netns exec {self.ns_name}"

    def ns_exec(self, cmd_string: str, **extra_args) -> int:
        """Run and execute an isolated command in the namespace."""
        cmd = self.get_ns_prefix() + " " + cmd_string
        # bash -c allows us to run multiple commands at once
        result = testutils.exec_process(cmd, **extra_args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to run command in namespace %s", self.ns_name)
        return result.returncode

    def ns_proc_open(self) -> testutils.Optional[testutils.subprocess.Popen]:
        """Open a bash process in the namespace and return the handle"""
        cmd = self.get_ns_prefix() + " bash"
        return testutils.open_process(cmd)

    def ns_proc_write(self, proc: testutils.subprocess.Popen, cmd: str) -> int:
        """Allows writing of a command to a given process. The command is NOT
        yet executed."""
        testutils.log.info("Writing %s ", cmd)
        try:
            proc.stdin.write(cmd)
        except IOError as exception:
            testutils.log.error("Error while writing to process\n%s", exception)
            return testutils.FAILURE
        return testutils.SUCCESS

    def ns_proc_append(self, proc: testutils.subprocess.Popen, cmd: str) -> int:
        """Append a command to an open process."""
        return self.ns_proc_write(proc, " && " + cmd)

    def ns_proc_close(self, proc: testutils.subprocess.Popen, **extra_args) -> int:
        """Close and actually run the process in the namespace. Returns the
        exit code."""
        testutils.log.info("Executing command: " + str(proc))
        result = testutils.run_process(proc, timeout=testutils.TIMEOUT, **extra_args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error(
                "Failed to execute the command sequence in namespace %s", self.ns_name
            )
        return result.returncode

    def _configure_bridge(self, br_name: str) -> int:
        """Set the bridge active. We also disable IPv6 to
        avoid ICMPv6 spam."""
        # We do not care about failures here
        self.ns_exec(f"ip link set dev {br_name} up")
        self.ns_exec(f"ip link set dev {br_name} mtu 9000")
        # Prevent the broadcasting of ipv6 link discovery messages
        self.ns_exec("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        self.ns_exec("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        # Also filter igmp packets, -w is necessary because of a race condition
        self.ns_exec("iptables -w -A OUTPUT -p 2 -j DROP")
        return testutils.SUCCESS

    def create_bridge(self) -> int:
        """Create the central bridge of the environment and configure it."""
        result = self.ns_exec(f"ip link add {self.br_name} type bridge")
        if result != testutils.SUCCESS:
            return result
        return self._configure_bridge(self.br_name)

    def _configure_bridge_port(self, port_name: str) -> int:
        """Set a bridge port active."""
        cmd = f"ip link set dev {port_name} up"
        result = self.ns_exec(cmd)
        if result != testutils.SUCCESS:
            return result
        cmd = f"ip link set dev {port_name} mtu 9000"
        return self.ns_exec(cmd)

    def attach_interfaces(self, num_ifaces: int) -> int:
        """Attach and initialize n interfaces to the central bridge of the
        namespace."""
        for index in range(num_ifaces):
            edge_veth = str(index)
            bridge_veth = f"br_{index}"
            result = self.ns_exec(f"ip link add {edge_veth} type veth peer name {bridge_veth}")
            if result != testutils.SUCCESS:
                return result
            # result = self.ns_exec("ip link set %s master %s" %
            #                       (edge_veth, self.br_name))
            # if result != testutils.SUCCESS:
            #     return result
            result = self._configure_bridge_port(edge_veth)
            if result != testutils.SUCCESS:
                return result
            result = self._configure_bridge_port(bridge_veth)
            if result != testutils.SUCCESS:
                return result
            # add interfaces to the list of existing bridge ports
            self.br_ports.append(bridge_veth)
            self.edge_ports.append(edge_veth)
        return testutils.SUCCESS

    def create_virtual_env(self, num_ifaces: int) -> int:
        """Create the namespace, the bridge, and attach interfaces all at
        once."""
        result = self.ns_init()
        if result != testutils.SUCCESS:
            return result
        result = self.create_bridge()
        if result != testutils.SUCCESS:
            return result
        testutils.log.info("Attaching %s interfaces...", num_ifaces)
        return self.attach_interfaces(num_ifaces)


def main(argv):
    """main"""
    # This is a simple test script which creates/deletes a virtual environment

    # Configure logging.
    logging.basicConfig(
        filename="ebpfenv.log",
        format="%(levelname)s:%(message)s",
        level=getattr(logging, logging.INFO),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s:%(message)s"))
    logging.getLogger().addHandler(stderr_log)

    bridge = Bridge("12345")
    bridge.create_virtual_env(5)
    bridge.ns_del()


if __name__ == "__main__":
    main(sys.argv)

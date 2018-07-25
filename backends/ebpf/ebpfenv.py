#!/usr/bin/env python
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

""" """

import os
import sys
from subprocess import Popen
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../../tools')
from testutils import *


class Bridge(object):

    def __init__(self, namespace, outputs, verbose):
        self.ns_name = namespace
        self.br_name = self.ns_name
        self.br_ports = []
        self.outputs = outputs      # contains standard and error output
        self.verbose = verbose      # contains standard and error output

    def ns_init(self):
        cmd = ["ip", "netns", "add", self.ns_name]
        errmsg = "Failed to create namespace:"
        result = run_timeout(True, cmd, TIMEOUT,
                             self.outputs, errmsg)
        self.ns_exec("ip link set dev lo up")
        return result

    def ns_del(self):
        self.ns_exec("ip link del dev %s" % (self.br_name))
        cmd = ["ip", "netns", "del", self.ns_name]
        errmsg = "Failed to delete namespace:"
        return run_timeout(True, cmd, TIMEOUT,
                           self.outputs, errmsg)

    def get_ns_prefix(self):
        # return ["ip", "netns", "exec", self.ns_name]
        return []

    def ns_exec(self, cmd_string):
        prefix = self.get_ns_prefix()
        prefix.extend(cmd_string.split())
        errmsg = "Failed to run command in namespace %s:" % self.ns_name
        run_timeout(self.verbose, prefix, TIMEOUT,
                    self.outputs, errmsg)

    def _configure_bridge(self, br_name):
        self.ns_exec("ip link set dev %s up" % br_name)
        self.ns_exec("tc qdisc add dev %s clsact" % br_name)
        self.ns_exec("sysctl -w net.ipv6.conf.all.accept_ra=0")
        self.ns_exec("sysctl -w net.ipv6.conf.%s.forwarding=0" % br_name)
        self.ns_exec("sysctl -w net.ipv6.conf.%s.accept_ra=0" % br_name)

    def create_bridge(self):
        self.ns_exec("ip link add %s type bridge" % self.br_name)
        self._configure_bridge(self.br_name)

    def _configure_bridge_port(self, port_name):
        self.ns_exec("ip link set dev %s up" % port_name)
        self.ns_exec("sysctl -w net.ipv6.conf.%s.accept_ra=0" % port_name)
        self.ns_exec("sysctl -w net.ipv6.conf.%s.forwarding=0" % port_name)
        self.ns_exec("tc qdisc add dev %s clsact" % port_name)

    def attach_interfaces(self, num_ifaces):
        for index in (range(num_ifaces)):
            edge_tap = "%s_%d" % (self.ns_name, index)
            self.ns_exec("ip link add link %s name %s"
                         " type macvtap mode vepa" % (self.br_name, edge_tap))
            self.br_ports.append(edge_tap)
            self._configure_bridge_port(edge_tap)

    def _load_tc(self, port_name, ebpf_obj):
        self.ns_exec("tc filter add dev %s ingress"
                     " bpf da obj %s section prog "
                     "verbose" % (port_name, ebpf_obj))
        self.ns_exec("tc filter add dev %s egress"
                     " bpf da obj %s section prog "
                     "verbose" % (port_name, ebpf_obj))

    def load_ebpf(self, prog):
        for port in self.br_ports:
            self._load_tc(port, prog)

    def check_ebpf_maps(self, path):
        return self.ns_exec("ls -1 %s " % path)

    def create_virtual_env(self, num_ifaces):
        self.ns_init()
        self.create_bridge()
        self.attach_interfaces(num_ifaces)


def main(argv):
    """ main """
    # Parse options and process argv
    outputs = {}
    outputs["stdout"] = sys.stdout
    outputs["stderr"] = sys.stderr
    bridge = Bridge("12345", outputs, True)
    # Run the test with the extracted options and modified argv
    bridge.create_virtual_env(2)
    os.system("ip netns exec 12345 xterm -e bash &")
    os.system("ip netns exec 12345 xterm -e bash &")
    os.system("ip netns exec 12345 xterm -e bash")
    bridge.ns_del()


if __name__ == "__main__":
    main(sys.argv)

# from scapy.all import *
# sniff(iface="br0", prn=lambda x: sendp(x, iface=x.sniffed_on))
# tc filter add dev br0 parent ffff: u32 match u32 0 0 action drop

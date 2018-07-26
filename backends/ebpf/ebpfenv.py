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

""" Virtual environment which models a simple bridge with n attached
    interfaces. The bridge runs in a completely isolated namespace.
    Allows loading and testing of eBPF programs. """

import os
import sys
from subprocess import Popen
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../../tools')
from testutils import *


class Bridge(object):

    def __init__(self, namespace, outputs, verbose):
        self.ns_name = namespace    # identifier of the namespace
        self.br_name = self.ns_name  # name of the central bridge
        self.br_ports = []          # list of bridge ports
        self.outputs = outputs      # contains standard and error output
        self.verbose = verbose      # do we want to be chatty?

    def ns_init(self):
        cmd = "ip netns add %s" % self.ns_name
        errmsg = "Failed to create namespace:"
        result = run_timeout(True, cmd, TIMEOUT,
                             self.outputs, errmsg)
        self.ns_exec("ip link set dev lo up")
        return result

    def ns_del(self):
        self.ns_exec("ip link del dev %s" % (self.br_name))
        cmd = "ip netns del %s" % self.ns_name
        errmsg = "Failed to delete namespace:"
        return run_timeout(True, cmd, TIMEOUT,
                           self.outputs, errmsg)

    def get_ns_prefix(self):
        return "ip netns exec %s" % self.ns_name

    def ns_exec(self, cmd_string):
        prefix = self.get_ns_prefix()
        # bash-c allows us to run multiple commands at once
        cmd = "%s bash -c \"%s\"" % (prefix, cmd_string)
        errmsg = "Failed to run command in namespace %s:" % self.ns_name
        return run_timeout(self.verbose, cmd, TIMEOUT,
                           self.outputs, errmsg)

    def _configure_bridge(self, br_name):
        cmd = "ip link set dev %s up && " % br_name
        # Add the qdisc. MUST be clsact layer.
        cmd += "tc qdisc add dev %s clsact && " % br_name
        # Prevent the broadcasting of link discovery messages
        cmd += "sysctl -w net.ipv6.conf.all.accept_ra=0 && "
        cmd += "sysctl -w net.ipv6.conf.%s.forwarding=0 && " % br_name
        cmd += "sysctl -w net.ipv6.conf.%s.accept_ra=0" % br_name
        return self.ns_exec(cmd)

    def create_bridge(self):
        result = self.ns_exec("ip link add %s type bridge" % self.br_name)
        if result != SUCCESS:
            return result
        return self._configure_bridge(self.br_name)

    def _configure_bridge_port(self, port_name):
        cmd = "ip link set dev %s up && " % port_name
        # prevent the broadcasting of link discovery messages
        cmd += "sysctl -w net.ipv6.conf.%s.accept_ra=0 && " % port_name
        cmd += "sysctl -w net.ipv6.conf.%s.forwarding=0 && " % port_name
        cmd += "tc qdisc add dev %s clsact" % port_name
        return self.ns_exec(cmd)

    def attach_interfaces(self, num_ifaces):
        for index in (range(num_ifaces)):
            edge_tap = "%s_%d" % (self.ns_name, index)
            self.ns_exec("ip link add link %s name %s"
                         " type macvtap mode vepa" % (self.br_name, edge_tap))
            self._configure_bridge_port(edge_tap)
            # add interface to the list of existing bridge ports
            self.br_ports.append(edge_tap)
        return SUCCESS

    def _load_tc(self, port_name, prog):
        # load the specified ebpf object to "port_name" ingress and egress
        # as a side-effect, this may create maps in /sys/fs/bpf/tc/globals
        cmd = ("tc filter add dev %s ingress"
               " bpf da obj %s section prog "
               "verbose && " % (port_name, prog))
        cmd += ("tc filter add dev %s egress"
                " bpf da obj %s section prog "
                "verbose" % (port_name, prog))
        return self.ns_exec(cmd)

    def get_load_ebpf_cmd(self, prog, load_bridge=False):
        cmd = ""
        if (load_bridge):
            cmd = ("tc filter add dev %s ingress"
                   " bpf da obj %s section prog "
                   "verbose && " % (self.br_name, prog))
            cmd += ("tc filter add dev %s egress"
                    " bpf da obj %s section prog "
                    "verbose " % (self.br_name, prog))
            return cmd
        else:
            for port in self.br_ports:
                cmd = ("tc filter add dev %s ingress"
                       " bpf da obj %s section prog "
                       "verbose && " % (port, prog))
                cmd += ("tc filter add dev %s egress"
                        " bpf da obj %s section prog "
                        "verbose " % (port, prog))
        return cmd

    def load_ebpf(self, prog, load_bridge):
        if (load_bridge):
            cmd = ("tc filter add dev %s ingress"
                   " bpf da obj %s section prog "
                   "verbose && " % (self.br_name, prog))
            cmd += ("tc filter add dev %s egress"
                    " bpf da obj %s section prog "
                    "verbose " % (self.br_name, prog))
            return cmd
        else:
            for port in self.br_ports:
                result = self._load_tc(port, prog)
                if result != SUCCESS:
                    return result
            return SUCCESS

    def check_ebpf_maps(self, path):
        # check if path has been created
        return self.ns_exec("ls -1 %s | wc -l" % path)

    def create_virtual_env(self, num_ifaces):
        self.ns_init()
        self.create_bridge()
        return self.attach_interfaces(num_ifaces)


def main(argv):
    """ main """
    # This is a simple test script which creates/deletes a virtual environment
    outputs = {}
    outputs["stdout"] = sys.stdout
    outputs["stderr"] = sys.stderr
    bridge = Bridge("12345", outputs, True)
    # Run the test with the extracted options and modified argv
    bridge.create_virtual_env(5)
    bridge.ns_del()


if __name__ == "__main__":
    main(sys.argv)

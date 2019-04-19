#!/usr/bin/env python2
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

import os
import sys
from subprocess import Popen, PIPE
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../../tools')
from testutils import *


class Bridge(object):

    def __init__(self, namespace, outputs, verbose):
        self.ns_name = namespace     # identifier of the namespace
        self.br_name = "core"        # name of the central bridge
        self.br_ports = []           # list of the veth pair bridge ports
        self.edge_ports = []         # list of the veth pair edge ports
        self.outputs = outputs       # contains standard and error output
        self.verbose = verbose       # do we want to be chatty?

    def ns_init(self):
        """ Initialize the namespace. """
        cmd = "ip netns add %s" % self.ns_name
        errmsg = "Failed to create namespace %s :" % self.ns_name
        result = run_timeout(self.verbose, cmd, TIMEOUT,
                             self.outputs, errmsg)
        self.ns_exec("ip link set dev lo up")
        return result

    def ns_del(self):
        """ Delete the namespace. """
        cmd = "ip netns del %s" % self.ns_name
        errmsg = "Failed to delete namespace %s :" % self.ns_name
        return run_timeout(self.verbose, cmd, TIMEOUT,
                           self.outputs, errmsg)

    def get_ns_prefix(self):
        """ Return the command prefix for the namespace of this bridge class.
            """
        return "ip netns exec %s" % self.ns_name

    def ns_exec(self, cmd_string):
        """ Run and execute an isolated command in the namespace. """
        prefix = self.get_ns_prefix()
        # bash -c allows us to run multiple commands at once
        cmd = "%s bash -c \"%s\"" % (prefix, cmd_string)
        errmsg = "Failed to run command %s in namespace %s:" % (
            cmd, self.ns_name)
        return run_timeout(self.verbose, cmd, TIMEOUT,
                           self.outputs, errmsg)

    def ns_proc_open(self):
        """ Open a bash process in the namespace and return the handle """
        cmd = self.get_ns_prefix() + " /bin/bash "
        return open_process(self.verbose, cmd, self.outputs)

    def ns_proc_write(self, proc, cmd):
        """ Allows writing of a command to a given process. The command is NOT
            yet executed. """
        report_output(self.outputs["stdout"],
                      self.verbose, "Writing %s " % cmd)
        try:
            proc.stdin.write(cmd)
        except IOError as e:
            err_text = "Error while writing to process"
            report_err(self.outputs["stdout"], err_text, e)
            return FAILURE
        return SUCCESS

    def ns_proc_append(self, proc, cmd):
        """ Append a command to an open process. """
        return self.ns_proc_write(proc, " && " + cmd)

    def ns_proc_close(self, proc):
        report_output(self.outputs["stdout"],
                      self.verbose, "Executing command.")
        """ Close and actually run the process in the namespace. Returns the
            exit code. """
        errmsg = ("Failed to execute the command"
                  " sequence in namespace %s" % self.ns_name)
        return run_process(self.verbose, proc, TIMEOUT, self.outputs, errmsg)

    def _configure_bridge(self, br_name):
        """ Set the bridge active. We also disable IPv6 to
            avoid ICMPv6 spam. """
        # We do not care about failures here
        self.ns_exec("ip link set dev %s up" % br_name)
        # Prevent the broadcasting of ipv6 link discovery messages
        self.ns_exec("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        self.ns_exec("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        return SUCCESS

    def create_bridge(self):
        """ Create the central bridge of the environment and configure it. """
        result = self.ns_exec("ip link add %s type bridge" % self.br_name)
        if result != SUCCESS:
            return result
        return self._configure_bridge(self.br_name)

    def _configure_bridge_port(self, port_name):
        """ Set a bridge port active. """
        cmd = "ip link set dev %s up" % port_name
        return self.ns_exec(cmd)

    def attach_interfaces(self, num_ifaces):
        """ Attach and initialize n interfaces to the central bridge of the
            namespace. """
        for index in (range(num_ifaces)):
            edge_veth = "%s" % index
            bridge_veth = "br_%s" % index
            result = self.ns_exec("ip link add %s type veth "
                                  "peer name %s" % (edge_veth, bridge_veth))
            if result != SUCCESS:
                return result
            result = self.ns_exec("ip link set %s master %s" %
                                  (edge_veth, self.br_name))
            if result != SUCCESS:
                return result
            result = self._configure_bridge_port(edge_veth)
            if result != SUCCESS:
                return result
            result = self._configure_bridge_port(bridge_veth)
            if result != SUCCESS:
                return result
            # add interfaces to the list of existing bridge ports
            self.br_ports.append(bridge_veth)
            self.edge_ports.append(edge_veth)
        return SUCCESS

    def create_virtual_env(self, num_ifaces):
        """ Create the namespace, the bridge, and attach interfaces all at
            once. """
        result = self.ns_init()
        if result != SUCCESS:
            return result
        result = self.create_bridge()
        if result != SUCCESS:
            return result
        report_output(self.outputs["stdout"],
                      self.verbose, "Attaching %d interfaces..." % num_ifaces)
        return self.attach_interfaces(num_ifaces)


def main(argv):
    """ main """
    # This is a simple test script which creates/deletes a virtual environment
    outputs = {}
    outputs["stdout"] = sys.stdout
    outputs["stderr"] = sys.stderr
    bridge = Bridge("12345", outputs, True)
    bridge.create_virtual_env(5)
    bridge.ns_del()


if __name__ == "__main__":
    main(sys.argv)

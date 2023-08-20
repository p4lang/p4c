#!/usr/bin/env python

# Copyright 2019 Orange
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

import os
import subprocess

import ptf
from ptf import config, testutils
from ptf.base_tests import BaseTest


class P4rtOVSBaseTest(BaseTest):
    def setUp(self):
        BaseTest.setUp(self)

        self.bridge = "br0"
        self.pipeline_id = 1
        self.map_id = 1

        test_params = testutils.test_params_get()
        print()
        print("You specified the following test-params when invoking ptf:")
        for k, v in test_params.items():
            print(k, ":\t\t\t", v)

        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()
        if config["log_dir"] != None:
            filename = os.path.join(config["log_dir"], str(self)) + ".pcap"
            self.dataplane.start_pcap(filename)

    def tearDown(self):
        BaseTest.tearDown(self)

    def invoke_ovs_cmd(self, cmd=[]):
        print("Invoking OVS command: ", " ".join(str(x) for x in cmd))
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = proc.communicate()

        if output:
            print("Output: ", output)

        if error:
            print("Error: ", error)

        return output

    def del_flows(self):
        cmd = ["sudo", "ovs-ofctl", "del-flows", self.bridge]
        self.invoke_ovs_cmd(cmd)

    def load_bpf_program(self, path_to_program):
        cmd = [
            "sudo",
            "ovs-ofctl",
            "load-bpf-prog",
            self.bridge,
            str(self.pipeline_id),
            path_to_program,
        ]
        self.invoke_ovs_cmd(cmd)

    def unload_bpf_program(self):
        cmd = [
            "sudo",
            "ovs-ofctl",
            "unload-bpf-prog",
            self.bridge,
            str(self.pipeline_id),
        ]
        self.invoke_ovs_cmd(cmd)

    def add_bpf_prog_flow(self, in_port, out_port):
        cmd = [
            "sudo",
            "ovs-ofctl",
            "add-flow",
            self.bridge,
            "in_port=%s,actions=prog:%s,output:%s" % (in_port, self.pipeline_id, out_port),
        ]
        self.invoke_ovs_cmd(cmd)

    def add_flow_normal(self):
        cmd = ["sudo", "ovs-ofctl", "add-flow", self.bridge, "actions=NORMAL"]
        self.invoke_ovs_cmd(cmd)

    def del_bpf_map(self, map_id=0, key="0 0 0 0"):
        cmd = [
            "sudo",
            "ovs-ofctl",
            "delete-bpf-map",
            self.bridge,
            str(self.pipeline_id),
            str(map_id),
        ]
        cmd = cmd + key.split()
        self.invoke_ovs_cmd(cmd)

    def update_bpf_map(self, map_id=0, key="0 0 0 0", value="0 0 0 0"):
        cmd = [
            "sudo",
            "ovs-ofctl",
            "update-bpf-map",
            self.bridge,
            str(self.pipeline_id),
            str(map_id),
            "key",
        ]
        cmd = cmd + key.split()
        cmd.append("value")
        cmd = cmd + value.split()
        self.invoke_ovs_cmd(cmd)

    def dump_bpf_map(self, map_id=0):
        cmd = [
            "sudo",
            "ovs-ofctl",
            "dump-bpf-map",
            self.bridge,
            str(self.pipeline_id),
            str(map_id),
        ]
        return self.invoke_ovs_cmd(cmd)

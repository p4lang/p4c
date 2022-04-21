# Copyright 2022-present Orange
# Copyright 2022-present Open Networking Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

""" This file implements a PTF test case abstraction for eBPF.
    Before each test case, the following steps are performed:
    1. Compile P4/PSA program to eBPF bytecode using 'make psa'.
    2. Load compiled eBPF programs to eBPF subsystem (psabpf-ctl load).
    3. Attach eBPF programs to interfaces used by PTF framework.

    After each test case, eBPF programs are detached from interfaces and removed from eBPF subsystem.

    This file also provides functions to manage P4-eBPF tables, clone sessions and multicast groups.
"""

import os
import logging
import json
import shlex
import subprocess
import ptf
import ptf.testutils as testutils

from ptf.base_tests import BaseTest

logger = logging.getLogger('eBPFTest')
if not len(logger.handlers):
    logger.addHandler(logging.StreamHandler())

TEST_PIPELINE_ID = 1
TEST_PIPELINE_MOUNT_PATH = "/sys/fs/bpf/pipeline{}".format(TEST_PIPELINE_ID)
PIPELINE_MAPS_MOUNT_PATH = "{}/maps".format(TEST_PIPELINE_MOUNT_PATH)

def xdp2tc_head_not_supported(cls):
    if cls.xdp2tc_mode(cls) == 'head':
        cls.skip = True
        cls.skip_reason = "not supported for xdp2tc=head"
    return cls


class P4EbpfTest(BaseTest):
    """
    Generates BPF bytecode from a P4 program and runs a PTF test.
    """

    skip = False
    skip_reason = ''
    switch_ns = 'test'
    p4_file_path = ""

    def setUp(self):
        super(P4EbpfTest, self).setUp()

        if self.skip:
            self.skipTest(self.skip_reason)

        if not os.path.exists(self.p4_file_path):
            self.fail("P4 program not found, no such file.")

        if not os.path.exists("ptf_out"):
            os.makedirs("ptf_out")

        head, tail = os.path.split(self.p4_file_path)
        filename = tail.split(".")[0]
        self.test_prog_image = os.path.join("ptf_out", filename + ".o")

        p4args = "--Wdisable=unused"
        if self.is_trace_logs_enabled():
            p4args += " --trace"

        if "xdp2tc" in testutils.test_params_get():
            p4args += " --xdp2tc=" + self.xdp2tc_mode()

        logger.info("P4ARGS=" + p4args)
        self.exec_cmd("make -f ../runtime/kernel.mk BPFOBJ={output} P4FILE={p4file} "
                      "ARGS=\"{cargs}\" P4C=p4c-ebpf P4ARGS=\"{p4args}\" psa".format(
                            output=self.test_prog_image,
                            p4file=self.p4_file_path,
                            cargs="-DPSA_PORT_RECIRCULATE=2",
                            p4args=p4args),
                      "Compilation error")

        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()
        logger.info("\nUsing test params: %s", testutils.test_params_get())
        if "namespace" in testutils.test_params_get():
            self.switch_ns = testutils.test_param_get("namespace")
        self.interfaces = testutils.test_param_get("interfaces").split(",")

        self.exec_ns_cmd("psabpf-ctl pipeline load id {} {}".format(TEST_PIPELINE_ID, self.test_prog_image), "Can't load programs into eBPF subsystem")

        for intf in self.interfaces:
            self.add_port(dev=intf)

    def tearDown(self):
        for intf in self.interfaces:
            self.del_port(intf)
        self.exec_ns_cmd("psabpf-ctl pipeline unload id {}".format(TEST_PIPELINE_ID))
        super(P4EbpfTest, self).tearDown()

    def exec_ns_cmd(self, command='echo me', do_fail=None):
        command = "nsenter --net=/var/run/netns/" + self.switch_ns + " " + command
        return self.exec_cmd(command, do_fail)

    def exec_cmd(self, command, do_fail=None):
        if isinstance(command, str):
            command = shlex.split(command)
        process = subprocess.Popen(command,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout_data, stderr_data = process.communicate()
        if stderr_data is None:
            stderr_data = ""
        if stdout_data is None:
            stdout_data = ""
        if process.returncode != 0:
            logger.info("Command failed: %s", command)
            logger.info("Return code: %d", process.returncode)
            logger.info("STDOUT: %s", stdout_data.decode("utf-8"))
            logger.info("STDERR: %s", stderr_data.decode("utf-8"))
            if do_fail:
                self.fail("Command failed (see above for details): {}".format(str(do_fail)))
        return process.returncode, stdout_data, stderr_data

    def add_port(self, dev):
        self.exec_ns_cmd("psabpf-ctl add-port pipe {} dev {}".format(TEST_PIPELINE_ID, dev))

    def del_port(self, dev):
        self.exec_ns_cmd("psabpf-ctl del-port pipe {} dev {}".format(TEST_PIPELINE_ID, dev))
        if dev.startswith("eth"):
            self.exec_cmd("psabpf-ctl del-port pipe {} dev s1-{}".format(TEST_PIPELINE_ID, dev))

    def read_map(self, name, key):
        cmd = "bpftool -j map lookup pinned {}/{} key {}".format(PIPELINE_MAPS_MOUNT_PATH, name, key)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Failed to read map {}".format(name))
        value = [format(int(v, 0), '02x') for v in json.loads(stdout)['value']]
        return ' '.join(value)

    def verify_map_entry(self, name, key, expected_value, mask=None):
        value = self.read_map(name, key)

        if "hex" in expected_value:
            expected_value = expected_value.replace("hex ", "")

        expected_value = "0x" + expected_value
        expected_value = expected_value.replace(" ", "")
        value = "0x" + value
        value = value.replace(" ", "")

        if mask:
            expected_value = int(expected_value, 0) & mask
            value = int(value, 0) & mask

        if expected_value != value:
            self.fail("Map {} key {} does not have correct value. Expected {}; got {}"
                      .format(name, key, expected_value, value))

    def xdp2tc_mode(self):
        return testutils.test_param_get('xdp2tc')

    def is_trace_logs_enabled(self):
        return testutils.test_param_get('trace') == 'True'

    def clone_session_create(self, id):
        self.exec_ns_cmd("psabpf-ctl clone-session create pipe {} id {}".format(TEST_PIPELINE_ID, id))

    def clone_session_add_member(self, clone_session, egress_port, instance=1, cos=0):
        self.exec_ns_cmd("psabpf-ctl clone-session add-member pipe {} id {} egress-port {} instance {} cos {}".format(
            TEST_PIPELINE_ID, clone_session, egress_port, instance, cos))

    def clone_session_delete(self, id):
        self.exec_ns_cmd("psabpf-ctl clone-session delete pipe {} id {}".format(TEST_PIPELINE_ID, id))

    def multicast_group_create(self, group):
        self.exec_ns_cmd("psabpf-ctl multicast-group create pipe {} id {}".format(TEST_PIPELINE_ID, group))

    def multicast_group_add_member(self, group, egress_port, instance=1):
        self.exec_ns_cmd("psabpf-ctl multicast-group add-member pipe {} id {} egress-port {} instance {}".format(
            TEST_PIPELINE_ID, group, egress_port, instance))

    def multicast_group_delete(self, group):
        self.exec_ns_cmd("psabpf-ctl multicast-group delete pipe {} id {}".format(TEST_PIPELINE_ID, group))

    def table_write(self, method, table, keys, action=0, data=None, priority=None, references=None):
        """
        Use table_add or table_update instead of this method
        """
        cmd = "psabpf-ctl table {} pipe {} {} ".format(method, TEST_PIPELINE_ID, table)
        if references:
            data = references
            cmd = cmd + "ref "
        else:
            cmd = cmd + "id {} ".format(action)
        cmd = cmd + "key "
        for k in keys:
            cmd = cmd + "{} ".format(k)
        if data:
            cmd = cmd + "data "
            for d in data:
                cmd = cmd + "{} ".format(d)
        if priority:
            cmd = cmd + "priority {}".format(priority)
        self.exec_ns_cmd(cmd, "Table {} failed".format(method))

    def table_add(self, table, keys, action=0, data=None, priority=None, references=None):
        self.table_write(method="add", table=table, keys=keys, action=action, data=data,
                         priority=priority, references=references)

    def table_update(self, table, keys, action=0, data=None, priority=None, references=None):
        self.table_write(method="update", table=table, keys=keys, action=action, data=data,
                         priority=priority, references=references)

    def table_delete(self, table, keys=None):
        cmd = "psabpf-ctl table delete pipe {} {} ".format(TEST_PIPELINE_ID, table)
        if keys:
            cmd = cmd + "key "
            for k in keys:
                cmd = cmd + "{} ".format(k)
        self.exec_ns_cmd(cmd, "Table delete failed")

    def action_selector_add_action(self, selector, action, data=None):
        cmd = "psabpf-ctl action-selector add_member pipe {} {} id {}".format(TEST_PIPELINE_ID, selector, action)
        if data:
            cmd = cmd + " data"
            for d in data:
                cmd = cmd + " {}".format(d)
        _, stdout, _ = self.exec_ns_cmd(cmd, "ActionSelector add_member failed")
        return int(stdout)

    def action_selector_create_empty_group(self, selector):
        cmd = "psabpf-ctl action-selector create_group pipe {} {}".format(TEST_PIPELINE_ID, selector)
        _, stdout, _ = self.exec_ns_cmd(cmd, "ActionSelector create_group failed")
        return int(stdout)

    def action_selector_add_member_to_group(self, selector, group_ref, member_ref):
        cmd = "psabpf-ctl action-selector add_to_group pipe {} {} {} to {}"\
            .format(TEST_PIPELINE_ID, selector, member_ref, group_ref)
        self.exec_ns_cmd(cmd, "ActionSelector add_to_group failed")

    def digest_get(self, name):
        cmd = "psabpf-ctl digest get pipe {} {}".format(TEST_PIPELINE_ID, name)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Digest get failed")
        return json.loads(stdout)['Digest'][name]['digests']

    def counter_get(self, name, keys=None):
        key_str = ""
        if keys:
            key_str = key_str + "key"
            for k in keys:
                key_str = key_str + " {}".format(k)
        cmd = "psabpf-ctl counter get pipe {} {} {}".format(TEST_PIPELINE_ID, name, key_str)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Counter get failed")
        return json.loads(stdout)['Counter'][name]

    def counter_verify(self, name, keys, bytes=None, packets=None):
        counter = self.counter_get(name, keys=keys)
        expected_type = ""
        if packets:
            expected_type = "PACKETS"
        if bytes:
            if packets:
                expected_type = expected_type + "_AND_"
            expected_type = expected_type + "BYTES"
        counter_type = counter["type"]
        if expected_type != counter_type:
            self.fail("Invalid counter type, expected: \"{}\", got \"{}\"".format(expected_type, counter_type))

        entries = counter["entries"]
        if len(entries) != 1:
            self.fail("expected one Counter entry")
        entry = entries[0]
        if bytes:
            counter_bytes = int(entry["value"]["bytes"], 0)
            if counter_bytes != bytes:
                self.fail("Invalid counter bytes, expected {}, got {}".format(bytes, counter_bytes))
        if packets:
            counter_packets = int(entry["value"]["packets"], 0)
            if counter_packets != packets:
                self.fail("Invalid counter packets, expected {}, got {}".format(packets, counter_packets))

    def meter_update(self, name, index, pir, pbs, cir, cbs):
        cmd = "psabpf-ctl meter update pipe {} {} " \
              "index {} {}:{} {}:{}".format(TEST_PIPELINE_ID, name,
                                            index, pir, pbs, cir, cbs)
        self.exec_ns_cmd(cmd, "Meter update failed")

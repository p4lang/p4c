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
    2. Load compiled eBPF programs to eBPF subsystem (nikss-ctl load).
    3. Attach eBPF programs to interfaces used by PTF framework.

    After each test case, eBPF programs are detached from interfaces and removed from eBPF subsystem.

    This file also provides functions to manage P4-eBPF tables, clone sessions and multicast groups.
"""

import json
import logging
import os
import shlex
import subprocess
import time

import ptf
import ptf.testutils as testutils
import pyroute2
from ptf.base_tests import BaseTest
from pyroute2 import NetNS

logger = logging.getLogger("eBPFTest")
if not len(logger.handlers):
    logger.addHandler(logging.StreamHandler())

TEST_PIPELINE_ID = 1
TEST_PIPELINE_MOUNT_PATH = "/sys/fs/bpf/pipeline{}".format(TEST_PIPELINE_ID)
PIPELINE_MAPS_MOUNT_PATH = "{}/maps".format(TEST_PIPELINE_MOUNT_PATH)

PORT0 = 0
PORT1 = 1
PORT2 = 2
# PTF_PORTS stores port numbers used by PTF test cases to send packets to the test switch.
# PTF_PORTS should only be used in PTF test cases to send/receive packets.
PTF_PORTS = [PORT0, PORT1, PORT2]

# DP_PORTS stores real interface numbers retrieved from OS.
# DP_PORTS corresponds to switch interfaces and are used as data plane port numbers inside P4 programs.
DP_PORTS = dict()


def tc_only(cls):
    if cls.is_xdp_test(cls):
        cls.skip = True
        cls.skip_reason = "not supported by XDP"
    return cls


def xdp2tc_head_not_supported(cls):
    if cls.xdp2tc_mode(cls) == "head":
        cls.skip = True
        cls.skip_reason = "not supported for xdp2tc=head"
    return cls


class P4EbpfTest(BaseTest):
    """
    Generates BPF bytecode from a P4 program and runs a PTF test.
    """

    skip = False
    skip_reason = ""
    switch_ns = "test"
    p4_file_path = ""
    p4c_additional_args = ""
    p4info_reference_file_path = ""

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
        logger.info("\nUsing test params: %s", testutils.test_params_get())
        if "namespace" in testutils.test_params_get():
            self.switch_ns = testutils.test_param_get("namespace")
        self.interfaces = testutils.test_param_get("interfaces").split(",")

        # force rebuild *.o file if compiler option has been changed for the same P4 file
        try:
            os.remove(os.path.join("ptf_out", filename + ".c"))
            os.remove(self.test_prog_image)
        except OSError:
            pass

        # fetch data plane ports from network namespace
        global DP_PORTS
        next_idx = 0
        for intf in self.interfaces:
            if intf != "psa_recirc":
                DP_PORTS[next_idx] = self.get_dataplane_port_number(intf)
                next_idx += 1

        p4args = "--Wdisable=unused --max-ternary-masks 3"
        for idx, dp_port in DP_PORTS.items():
            p4args += " -DPORT{}={}".format(idx, dp_port)
        p4args += " -DPSA_RECIRC={}".format(self.get_dataplane_port_number("psa_recirc"))
        if self.is_trace_logs_enabled():
            p4args += " --trace"

        if "xdp2tc" in testutils.test_params_get():
            p4args += " --xdp2tc=" + self.xdp2tc_mode()
        if self.is_xdp_test():
            p4args += " --xdp"

        if self.p4info_reference_file_path:
            self.p4info_p4c_generated = os.path.join("ptf_out", filename + ".p4info.txt")
            p4args += " --p4runtime-files {}".format(self.p4info_p4c_generated)

        p4args = p4args + " " + self.p4c_additional_args

        logger.info("P4ARGS=" + p4args)
        self.exec_cmd(
            "make -f ../runtime/kernel.mk BPFOBJ={output} P4FILE={p4file} "
            "ARGS=\"{cargs}\" P4C=p4c-ebpf P4ARGS=\"{p4args}\" psa".format(
                output=self.test_prog_image,
                p4file=self.p4_file_path,
                cargs="-DPSA_PORT_RECIRCULATE={}".format(
                    self.get_dataplane_port_number("psa_recirc")
                ),
                p4args=p4args,
            ),
            "Compilation error",
        )

        if self.p4info_reference_file_path:
            self.exec_cmd(
                "diff -B -u -w {expected} {produced}".format(
                    expected=self.p4info_reference_file_path,
                    produced=self.p4info_p4c_generated,
                ),
                "Compiler produced incorrect P4info file",
            )
            logger.info("Generated P4info file is correct")

        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()

        self.exec_ns_cmd(
            "nikss-ctl pipeline load id {} {}".format(TEST_PIPELINE_ID, self.test_prog_image),
            "Can't load programs into eBPF subsystem",
        )

        for intf in self.interfaces:
            # Recirculation not supported yet in XDP mode
            if intf == "psa_recirc" and self.is_xdp_test():
                continue
            self.add_port(dev=intf)

    def tearDown(self):
        for intf in self.interfaces:
            # Recirculation not supported yet in XDP mode
            if intf == "psa_recirc" and self.is_xdp_test():
                continue
            self.del_port(intf)
        self.exec_ns_cmd("nikss-ctl pipeline unload id {}".format(TEST_PIPELINE_ID))
        super(P4EbpfTest, self).tearDown()

    def get_dataplane_port_number(self, name):
        with pyroute2.NetNS(self.switch_ns) as netns:
            idx = netns.link_lookup(ifname=name)
            return idx[0]

    def exec_ns_cmd(self, command="echo me", do_fail=None):
        command = "nsenter --net=/var/run/netns/" + self.switch_ns + " " + command
        return self.exec_cmd(command, do_fail)

    def exec_cmd(self, command, do_fail=None):
        if isinstance(command, str):
            command = shlex.split(command)
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
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
        self.exec_ns_cmd("nikss-ctl add-port pipe {} dev {}".format(TEST_PIPELINE_ID, dev))
        # Add dummy XDP program on the second end of veth pair
        if dev.startswith("eth") and self.is_xdp_test():
            self.exec_cmd(
                "ip link set dev s1-{} xdp pinned {}/{}".format(
                    dev, TEST_PIPELINE_MOUNT_PATH, "xdp_redirect_dummy_sec"
                )
            )

    def del_port(self, dev):
        self.exec_ns_cmd("nikss-ctl del-port pipe {} dev {}".format(TEST_PIPELINE_ID, dev))
        if dev.startswith("eth"):
            self.exec_cmd("nikss-ctl del-port pipe {} dev s1-{}".format(TEST_PIPELINE_ID, dev))

    def read_map(self, name, key):
        cmd = "bpftool -j map lookup pinned {}/{} key {}".format(
            PIPELINE_MAPS_MOUNT_PATH, name, key
        )
        _, stdout, _ = self.exec_ns_cmd(cmd, "Failed to read map {}".format(name))
        value = [format(int(v, 0), "02x") for v in json.loads(stdout)["value"]]
        return " ".join(value)

    def verify_map_entry(self, name, key, expected_value, mask=None):
        value = self.read_map(name, key)

        if "hex" in expected_value:
            expected_value = expected_value.replace("hex ", "")

        expected_value = "0x" + expected_value
        expected_value = expected_value.replace(" ", "")
        value = "0x" + value
        value = value.replace(" ", "")

        if mask:
            expected_value = int(expected_value, 0) & int(mask, 0)
            value = int(value, 0) & int(mask, 0)

        if expected_value != value:
            self.fail(
                "Map {} key {} does not have correct value. Expected {}; got {}".format(
                    name, key, expected_value, value
                )
            )

    def xdp2tc_mode(self):
        return testutils.test_param_get("xdp2tc")

    def is_xdp_test(self):
        return testutils.test_param_get("xdp") == "True"

    def is_trace_logs_enabled(self):
        return testutils.test_param_get("trace") == "True"

    def clone_session_create(self, id):
        self.exec_ns_cmd(
            "nikss-ctl clone-session create pipe {} id {}".format(TEST_PIPELINE_ID, id)
        )

    def clone_session_add_member(self, clone_session, egress_port, instance=1, cos=0):
        self.exec_ns_cmd(
            "nikss-ctl clone-session add-member pipe {} id {} egress-port {} instance {} cos {}".format(
                TEST_PIPELINE_ID, clone_session, egress_port, instance, cos
            )
        )

    def clone_session_delete(self, id):
        self.exec_ns_cmd(
            "nikss-ctl clone-session delete pipe {} id {}".format(TEST_PIPELINE_ID, id)
        )

    def multicast_group_create(self, group):
        self.exec_ns_cmd(
            "nikss-ctl multicast-group create pipe {} id {}".format(TEST_PIPELINE_ID, group)
        )

    def multicast_group_add_member(self, group, egress_port, instance=1):
        self.exec_ns_cmd(
            "nikss-ctl multicast-group add-member pipe {} id {} egress-port {} instance {}".format(
                TEST_PIPELINE_ID, group, egress_port, instance
            )
        )

    def multicast_group_delete(self, group):
        self.exec_ns_cmd(
            "nikss-ctl multicast-group delete pipe {} id {}".format(TEST_PIPELINE_ID, group)
        )

    def _table_create_str_from_data(self, data, counters, meters):
        """Creates string from action data, direct counters and direct meters
        which can be passed to the nikss-cli as an argument.
        """
        s = ""
        if data or counters or meters:
            s = "data "
            if data:
                for d in data:
                    s = s + "{} ".format(d)
            if counters:
                for k, v in counters.items():
                    value = ""
                    bytes_cnt = v.get("bytes", None)
                    packets_cnt = v.get("packets", None)
                    if bytes_cnt is not None:
                        value = "{}".format(bytes_cnt)
                    if packets_cnt is not None:
                        if bytes_cnt is not None:
                            value = value + ":"
                        value = value + "{}".format(packets_cnt)
                    s = s + "counter {} {} ".format(k, value)
            if meters:
                for k, v in meters.items():
                    s = s + "meter {} {}:{} {}:{} ".format(
                        k, v["pir"], v["pbs"], v["cir"], v["cbs"]
                    )
        return s

    def _table_create_str_from_(self, name="key", value=None):
        """
        Creates a string from value (a list of fields)
        which can be passed to the nikss-cli as an argument.
        """
        s = name + " none"
        if value:
            s = name + " "
            for field in value:
                s = s + "{} ".format(field)
        return s

    def _table_create_str_from_key(self, key):
        return self._table_create_str_from_(name="key", value=key)

    def _table_create_str_from_index(self, index):
        return self._table_create_str_from_(name="index", value=index)

    def _table_create_str_from_value(self, value):
        return self._table_create_str_from_(name="value", value=value)

    def _table_create_str_from_action(self, action):
        if isinstance(action, int):
            return "action id {} ".format(action)
        else:
            return "action name {} ".format(action)

    def table_write(
        self,
        method,
        table,
        key,
        action=0,
        data=None,
        priority=None,
        references=None,
        counters=None,
        meters=None,
    ):
        """
        Use table_add or table_update instead of this method.
        """
        cmd = "nikss-ctl table {} pipe {} {} ".format(method, TEST_PIPELINE_ID, table)
        if references:
            data = references
            cmd = cmd + "ref "
        else:
            cmd = cmd + self._table_create_str_from_action(action)
        cmd = cmd + self._table_create_str_from_key(key=key)
        cmd = cmd + self._table_create_str_from_data(data=data, counters=counters, meters=meters)
        if priority:
            cmd = cmd + "priority {}".format(priority)
        self.exec_ns_cmd(cmd, "Table {} failed".format(method))

    def table_add(
        self,
        table,
        key,
        action=0,
        data=None,
        priority=None,
        references=None,
        counters=None,
        meters=None,
    ):
        """Adds a new entry to a table.
        :param table: Table name.
        :param key: List of key fields, each field must be convertible to string.
        :param action: Action ID in the dataplane.
        :param data: List of action parameters.
        :param priority: Priority of the new entry.
        :param references: List of references for indirect table (parameter data is ignored then).
        :param counters: Dictionary of counter's names (key) and dictionary of counter value (value).
            Inner dictionary can have two entries: "bytes", "packets"
        :param meters: Dictionary of meter's names (key) and dictionary of meter value (value).
            Inner dictionary must have four entries: "pir", "pbs", "cir", "cbs"
        """
        self.table_write(
            method="add",
            table=table,
            key=key,
            action=action,
            data=data,
            priority=priority,
            references=references,
            counters=counters,
            meters=meters,
        )

    def table_update(
        self,
        table,
        key,
        action=0,
        data=None,
        priority=None,
        references=None,
        counters=None,
        meters=None,
    ):
        """See documentation for table_add. This method updates existing entry instead of new one."""
        self.table_write(
            method="update",
            table=table,
            key=key,
            action=action,
            data=data,
            priority=priority,
            references=references,
            counters=counters,
            meters=meters,
        )

    def table_delete(self, table, key=None):
        """Deletes existing table entry"""
        cmd = "nikss-ctl table delete pipe {} {} ".format(TEST_PIPELINE_ID, table)
        if key:
            cmd = cmd + self._table_create_str_from_key(key)
        self.exec_ns_cmd(cmd, "Table delete failed")

    def table_set_default(self, table, action=0, data=None, counters=None, meters=None):
        """Sets default action for table. For parameters documentation see `table_add` method."""
        cmd = "nikss-ctl table default set pipe {} {} ".format(TEST_PIPELINE_ID, table)
        cmd = cmd + self._table_create_str_from_action(action)
        cmd = cmd + self._table_create_str_from_data(data=data, counters=counters, meters=meters)
        self.exec_ns_cmd(cmd, "Table set default entry failed")

    def table_get(self, table, key=None, indirect=False):
        """Returns JSON containing parsed table entry - action data, meters, counters.
        If table has an implementation, set param `indirect` to True.
        """
        cmd = "nikss-ctl table get pipe {} {} ".format(TEST_PIPELINE_ID, table)
        if indirect:
            # TODO: cmd = cmd + "ref "
            self.fail("support for indirect table is not implemented yet")
        if key:
            cmd = cmd + self._table_create_str_from_key(key=key)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Table get entry failed")
        return json.loads(stdout)[table]

    def table_verify(
        self,
        table,
        key,
        action=0,
        priority=None,
        data=None,
        references=None,
        counters=None,
        meters=None,
    ):
        """Verify that values in table entry fields are equal to provided arguments. For parameters
        documentation see `table_add` method. Field not referenced by any argument will not be tested.
        """
        json_data = self.table_get(table=table, key=key, indirect=references)
        entries = json_data["entries"]
        if len(entries) != 1:
            self.fail("Expected 1 table entry to verify")
        entry = entries[0]

        if action is not None:
            if action != entry["action"]["id"]:
                self.fail(
                    "Invalid action ID: expected {}, got {}".format(action, entry["action"]["id"])
                )
        if priority is not None:
            if priority != entry["priority"]:
                self.fail(
                    "Invalid priority: expected {}, got {}".format(priority, entry["priority"])
                )
        if data:
            action_params = entry["action"]["parameters"]
            if len(action_params) != len(data):
                self.fail(
                    "Invalid number of action parameters: expected {}, got {}".format(
                        len(data), len(action_params)
                    )
                )
            for k, v in enumerate(data):
                if v != int(action_params[k]["value"], 0):
                    self.fail(
                        "Invalid action parameter {} (id {}): expected {}, got {}".format(
                            action_params[k]["name"],
                            k,
                            v,
                            int(action_params[k]["value"], 0),
                        )
                    )
        if references:
            self.fail("Support for table references is not implemented yet")
        if counters:
            for k, v in counters.items():
                type = json_data["DirectCounter"][k]["type"]
                entry_value = entry["DirectCounter"][k]
                self._do_counter_verify(
                    bytes=v.get("bytes", None),
                    packets=v.get("packets", None),
                    entry_value=entry_value,
                    counter_type=type,
                )
        if meters:
            self.fail(
                "Support for DirectMeter is not implemented yet (nikss doesn't return internal"
                " state of meter if you need it)"
            )

    def action_selector_add_action(self, selector, action, data=None):
        cmd = "nikss-ctl action-selector add-member pipe {} {} ".format(TEST_PIPELINE_ID, selector)
        cmd = cmd + self._table_create_str_from_action(action)
        cmd = cmd + self._table_create_str_from_data(data=data, counters=None, meters=None)
        _, stdout, _ = self.exec_ns_cmd(cmd, "ActionSelector add-member failed")
        return json.loads(stdout)[selector]["added_member_ref"]

    def action_selector_create_empty_group(self, selector):
        cmd = "nikss-ctl action-selector create-group pipe {} {}".format(TEST_PIPELINE_ID, selector)
        _, stdout, _ = self.exec_ns_cmd(cmd, "ActionSelector create-group failed")
        return json.loads(stdout)[selector]["added_group_ref"]

    def action_selector_add_member_to_group(self, selector, group_ref, member_ref):
        cmd = "nikss-ctl action-selector add-to-group pipe {} {} {} to {}".format(
            TEST_PIPELINE_ID, selector, member_ref, group_ref
        )
        self.exec_ns_cmd(cmd, "ActionSelector add-to-group failed")

    def action_profile_add_action(self, ap, action, data=None):
        cmd = "nikss-ctl action-profile add-member pipe {} {} ".format(TEST_PIPELINE_ID, ap)
        cmd = cmd + self._table_create_str_from_action(action)
        cmd = cmd + self._table_create_str_from_data(data=data, counters=None, meters=None)
        _, stdout, _ = self.exec_ns_cmd(cmd, "ActionProfile add-member failed")
        return json.loads(stdout)[ap]["added_member_ref"]

    def digest_get(self, name):
        # Possible race conditions: many test cases may send packet and just
        # after that try to read digest message. In rare case packet can be
        # not yet processed so digest is not yet available.
        time.sleep(0.1)
        cmd = "nikss-ctl digest get-all pipe {} {}".format(TEST_PIPELINE_ID, name)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Digest get failed")
        return json.loads(stdout)[name]["digests"]

    def counter_get(self, name, key=None):
        key_str = self._table_create_str_from_key(key=key)
        cmd = "nikss-ctl counter get pipe {} {} {}".format(TEST_PIPELINE_ID, name, key_str)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Counter get failed")
        return json.loads(stdout)[name]

    def _do_counter_verify(self, bytes, packets, entry_value, counter_type):
        """Verify counter value and type. Use `counter_verify` or `table_verify` instead."""
        expected_type = ""
        if packets is not None:
            expected_type = "PACKETS"
        if bytes is not None:
            if packets is not None:
                expected_type = expected_type + "_AND_"
            expected_type = expected_type + "BYTES"
        if expected_type != counter_type:
            self.fail(
                'Invalid counter type, expected: "{}", got "{}"'.format(expected_type, counter_type)
            )
        if bytes is not None:
            counter_bytes = int(entry_value["bytes"], 0)
            if counter_bytes != bytes:
                self.fail("Invalid counter bytes, expected {}, got {}".format(bytes, counter_bytes))
        if packets is not None:
            counter_packets = int(entry_value["packets"], 0)
            if counter_packets != packets:
                self.fail(
                    "Invalid counter packets, expected {}, got {}".format(packets, counter_packets)
                )

    def counter_verify(self, name, key, bytes=None, packets=None):
        counter = self.counter_get(name, key=key)
        entries = counter["entries"]
        if len(entries) != 1:
            self.fail("expected one Counter entry")
        entry = entries[0]
        self._do_counter_verify(
            bytes=bytes,
            packets=packets,
            entry_value=entry["value"],
            counter_type=counter["type"],
        )

    def meter_get(self, name, index=None):
        cmd = "nikss-ctl meter get pipe {} {}".format(TEST_PIPELINE_ID, name)
        if index:
            cmd = cmd + " index {}".format(index)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Meter get failed")
        return json.loads(stdout)[name]["entries"]

    def meter_update(self, name, index, pir, pbs, cir, cbs):
        cmd = "nikss-ctl meter update pipe {} {} index {} {}:{} {}:{}".format(
            TEST_PIPELINE_ID, name, index, pir, pbs, cir, cbs
        )
        self.exec_ns_cmd(cmd, "Meter update failed")

    def register_get(self, name, index=None):
        index_str = self._table_create_str_from_index(index=index)
        cmd = "nikss-ctl register get pipe {} {} {}".format(TEST_PIPELINE_ID, name, index_str)
        _, stdout, _ = self.exec_ns_cmd(cmd, "Register get failed")
        return json.loads(stdout)[name]

    def register_set(self, name, index=None, value=None):
        index_str = self._table_create_str_from_index(index=index)
        value_str = self._table_create_str_from_value(value=value)
        cmd = "nikss-ctl register set pipe {} {} {} {}".format(
            TEST_PIPELINE_ID, name, index_str, value_str
        )
        _, stdout, _ = self.exec_ns_cmd(cmd, "Register set failed")

    def register_verify(self, name, index, expected_value):
        """
        Verifies a register value.
        :param name: Register name
        :param index: A list of hex string
        :param expected_value: A list of hex string
        """
        entries = self.register_get(name, index=index)
        if len(entries) != 1:
            self.fail("expected one Register entry")
        entry = entries[0]
        reg_value = entry["value"]
        for idx, field_value in enumerate(reg_value.values()):
            field_value = int(field_value, 0)
            expected_field_value = int(expected_value[idx], 0)
            if field_value != expected_field_value:
                self.fail(
                    "Invalid register value, expected {}, got {}".format(
                        expected_field_value, field_value
                    )
                )

    def value_set_insert(self, name, value):
        cmd = "nikss-ctl value-set insert pipe {} {} ".format(TEST_PIPELINE_ID, name)
        cmd = cmd + self._table_create_str_from_value(value=value)
        self.exec_ns_cmd(cmd, "Value_set insert failed")

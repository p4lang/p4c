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
"""Represents the call which contains all methods needed to execute a TC
 test. Currently five phases are defined:
1. Invokes the specified compiler on a provided p4 file.
2. Parses an stf file and generates a pcap output.
3. Loads the generated template nd the eBPF binaries
4. Feeds the generated pcap test packets into ptf.pcap_writer.PcapWriter
5. Evaluates the output with the expected result from the .stf file
"""

import difflib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import time
from glob import glob
from pathlib import Path

from tcenv import Virtme

# path to the tools folder of the compiler
# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../tools")))
sys.path.append(str(FILE_DIR.joinpath("../ebpf/targets")))

import testutils
from ptf.pcap_writer import LINKTYPE_ETHERNET, PcapWriter, rdpcap
from stf.stf_parser import STFParser

PCAP_PREFIX = "pcap"  # match pattern
PCAP_SUFFIX = ".pcap"  # could also be ".pcapng"


class P4TCCommand(object):
    def __init__(self, pipe_name, cmd_type, table, action, priority="", match=[], extra=""):
        # Dir in which all files are stored.
        self.pipe_name = pipe_name
        # Dir in which all files are stored.
        self.cmd_type = cmd_type
        # Contains meta information.
        self.table = table
        # Contains meta information.
        self.action = action
        # Template to generate a filter.
        self.priority = priority
        # Contains standard and error output.
        self.match = match
        # Could also be "pcapng".
        self.extra = extra


def parse_stf_file(raw_stf, pipe_name):
    parser = STFParser()
    stf_str = raw_stf.read()
    stf_map, errs = parser.parse(stf_str)
    input_pkts = {}
    cmds = []
    expected = {}
    for stf_entry in stf_map:
        if stf_entry[0] == "packet":
            interface = int(stf_entry[1])
            data = stf_entry[2]
            input_pkts.setdefault(interface, []).append(bytes.fromhex("".join(data.split())))
            expected.setdefault(interface, {})
            if data != "":
                expected[interface]["any"] = False
                expected[interface].setdefault("pkts", []).append(data)
            else:
                expected[interface]["any"] = True
        elif stf_entry[0] == "expect":
            interface = int(stf_entry[1])
            pkt_data = stf_entry[2]
            expected.setdefault(interface, {})
            if pkt_data != "":
                expected[interface]["any"] = False
                expected[interface].setdefault("pkts", []).append(pkt_data)
            else:
                expected[interface]["any"] = True
        elif stf_entry[0] == "add":
            cmd = P4TCCommand(
                pipe_name=pipe_name,
                cmd_type=stf_entry[0],
                table=stf_entry[1],
                priority=stf_entry[2],
                match=stf_entry[3],
                action=stf_entry[4],
                extra=stf_entry[5],
            )
            cmds.append(cmd)
    return input_pkts, cmds, expected


def parse_p4_json(json_file):
    data = None
    with open(json_file, 'r') as file:
        data = json.load(file)
    return data


def find_action_json(json_table, action_name):
    for action in json_table['actions']:
        if action['name'] == action_name:
            return action


def int_to_mac(int_val):
    hex_str = '{:012x}'.format(int_val)
    mac = ":".join(hex_str[i : i + 2] for i in range(0, 12, 2))
    return mac


def map_value(value, port_mapping, json_type):
    if json_type == 'dev':
        return port_mapping[int(value, 16)]

    if json_type == 'macaddr':
        return int_to_mac(int(value, 16))

    return value


def build_runtime_file(rules_file, json_file, cmds, port_mapping):
    json_data = parse_p4_json(json_file)
    for index, cmd in enumerate(cmds):
        generated = "$TC p4ctrl create "
        table_name = cmd.table
        table_name = table_name.replace(".", "/")
        generated += f"{cmd.pipe_name}/table/{table_name} "
        json_tables = json_data['tables']
        for table in json_tables:
            if table_name == table['name']:
                json_table = table

        if not json_table:
            return testutils.FAILURE

        if cmd.cmd_type == "add":
            for i, key_field_val in enumerate(cmd.match):
                key_json = json_table['keyfields'][i]
                field = key_field_val[0]
                # Support for LPM key
                generated += f"{field} "
                if isinstance(key_field_val[1], tuple):
                    key_field_val = map_value(key_field_val[1][0], port_mapping, key_json['type'])
                    generated += f"{key_field_val[1][0]}/{key_field_val[1][1]} "
                else:
                    key_field_val = map_value(key_field_val[1], port_mapping, key_json['type'])
                    generated += f"{key_field_val} "
        if cmd.action[0] != "_NoAction":
            action_name = cmd.action[0].split(".")[1]
            action_json = find_action_json(json_table, cmd.action[0].replace(".", "/"))
            generated += f"action {action_name} "
            for i, val_field in enumerate(cmd.action[1]):
                param_json = action_json['params'][i]
                param_val = map_value(val_field[1], port_mapping, param_json['type'])
                generated += f"param {val_field[0]} {param_val} "
    file = open(rules_file, "w")
    file.write(generated)
    st = os.stat(rules_file)
    os.chmod(rules_file, st.st_mode | stat.S_IEXEC)

    return testutils.SUCCESS


class Bridge:
    def __init__(self, runtimedir, bridge_ip):
        self.spawn_bridge_script = f"{runtimedir}/spawn-bridge"
        self.del_bridge_script = f"{runtimedir}/del-bridge"
        self.bridge_ip = bridge_ip
        self.is_spawned = False

    def del_bridge(self):
        result = testutils.exec_process(self.del_bridge_script)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to delete bridge")

        self.is_spawned = True

        return result

    def __del__(self):
        if self.is_spawned:
            self.del_bridge()

    def spawn_bridge(self):
        self.is_spawned = True
        result = testutils.exec_process(f"{self.spawn_bridge_script} {self.bridge_ip}")
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to spawn bridge")

        return result.returncode


def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in str(p4filename)


class TCInfra:
    def __init__(self, tmpdir, options, template):
        # Dir in which all files are stored.
        self.tmpdir = tmpdir
        # Contains meta information.
        self.options = options
        # Template to generate a filter.
        self.template = template
        # Expected packets per interface.
        self.expected = {}
        # Location of the runtime folder.
        self.runtimedir = options.runtimedir
        # Location of the p4c compiler binary.
        self.compiler = self.options.compiler
        self.spawn_script = f"{self.runtimedir}/spawn-vm"
        self.post_boot_script = "vm-post-boot"
        self.send_packet_script = "send_packet"
        self.bridge_ip = "10.10.11.1"
        tmpdir_basename = os.path.basename(tmpdir)

        self.virtme = Virtme(
            "p4node",
            self.runtimedir,
            "iproute2-p4tc-pub",
            self.spawn_script,
            self.post_boot_script,
            tmpdir_basename,
            options.verbose,
        )

        self.base_template = os.path.basename(self.template)
        # self.outputdir = f"{self.runtimedir}/generated"
        self.outputdir = tmpdir
        self.bridge = Bridge(options.runtimedir, self.bridge_ip)
        self.rules_file = ''

    def __del__(self):
        if self.options.cleanupTmp:
            shutil.rmtree(self.outputdir)

    def filename(self, prefix, interface, direction):
        """Constructs the pcap filename from the given interface and
        packet stream direction. For example "pcap1_out.pcap" implies
        that the given stream contains tx packets from interface 1"""
        return prefix + "/" + PCAP_PREFIX + str(interface) + "_" + direction + PCAP_SUFFIX

    def interface_of_filename(self, f):
        """Extracts the interface name out of a pcap filename"""
        return int(os.path.basename(f).rstrip(PCAP_SUFFIX).lstrip(PCAP_PREFIX).rsplit("_", 1)[0])

    def _send_pcap_files(self, iface_pkts_map):
        """Writes the collected packets to their respective interfaces.
        This is done by creating a pcap file with the corresponding name."""
        result = testutils.SUCCESS
        for iface, pkts in iface_pkts_map.items():
            direction = "in"
            infile = self.filename(self.runtimedir, iface, direction)
            fp = PcapWriter(infile, linktype=LINKTYPE_ETHERNET)
            for pkt_data in pkts:
                try:
                    fp.write(pkt_data)
                except ValueError:
                    testutils.log.error(f"Invalid packet data {pkt_data}")
                    return testutils.ProcessResult("", testutils.FAILURE)
            fp.flush()
            fp.close()

            args = [infile, self.virtme.port_mapping[iface]]

            result = self.virtme.run_script(self.send_packet_script, args)
            if result != testutils.SUCCESS:
                return result

        return result

    def compile_p4(self):
        # Use clang to compile the generated C code to a LLVM IR
        args = "make -f tc.mk "
        # target makefile
        # Source folder of the makefile
        args += f"-C {self.runtimedir} "
        args += f"TEMPLATE={self.base_template} "
        args += f"P4_FILE={self.options.p4filename} "
        args += f"OUTPUT_DIR={self.outputdir} "
        args += f"P4C={self.compiler} CLANG={self.options.clang}"
        # add the folder local to the P4 file to the list of includes
        args += f" INCLUDES+=-I{os.path.dirname(self.options.p4filename)}"
        result = testutils.exec_process(args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to compile P4 program")

        returncode = result.returncode

        expected_error = isError(self.options.p4filename)
        if expected_error:
            if result.returncode == testutils.SUCCESS:
                returncode = testutils.FAILURE
            else:
                returncode = testutils.SUCCESS

        return returncode

    def __compare_compiled_files(self, produced, expected):
        if self.options.verbose:
            print("Comparing ", produced, " and ", expected)
        if produced.endswith(".c"):
            sedcommand = "sed -i.bak '1d;2d' " + produced
            subprocess.call(sedcommand, shell=True)
        if produced.endswith(".h"):
            sedcommand = "sed -i.bak '1d;2d' " + produced
            subprocess.call(sedcommand, shell=True)
        sedcommand = "sed -i -e 's/[a-zA-Z0-9_\/\-]*testdata\///g' " + produced
        subprocess.call(sedcommand, shell=True)

        result = testutils.SUCCESS
        if self.options.replace:
            if self.options.verbose:
                print("Saving new version of ", expected)
            shutil.copy2(produced, expected)
            return result

        diff = difflib.Differ().compare(open(produced).readlines(), open(expected).readlines())

        message = ""
        for l in diff:
            if l[0] == ' ':
                continue
            result = testutils.FAILURE
            message += l

        if message != "":
            if self.options.verbose:
                print("Files ", produced, " and ", expected, " differ:", file=sys.stderr)
                print(message, file=sys.stderr)

        return result

    def compare_compiled_files(self):
        files = os.listdir(self.outputdir)
        dirname = os.path.dirname(self.options.p4filename)
        expected_dirname = dirname + "_outputs"
        for file in files:
            _, extension = os.path.splitext(file)
            # Skip generated object files
            if extension == ".o":
                continue
            if self.options.verbose:
                print("Checking", file)
            produced = self.outputdir + "/" + file
            expected = expected_dirname + "/" + file
            if not os.path.isfile(expected):
                if self.options.verbose:
                    print("Expected file does not exist; creating", expected)
                shutil.copy2(produced, expected)
            else:
                result = self.__compare_compiled_files(produced, expected)
                if result != testutils.SUCCESS:
                    return result

        return testutils.SUCCESS

    def _kill_processes(self, pids):
        for pid in pids:
            # kill pidess, 15 is SIGTERM
            result = self.virtme.run(f"kill -s 15 {pid}")
            if result != testutils.SUCCESS:
                return result.returncode

        return testutils.SUCCESS

    def load_filter_cmd(self, ns):
        # Load the specified eBPF objects to "block" ingress
        # As a side-effect, this may create maps in /sys/fs/bpf/

        cmd = (
            "{tc} filter add "
            f"block {ns.block_num} ingress protocol all prio 10 "
            f"p4 pname {self.base_template} "
            "action bpf obj {introspection}/"
            f"{self.base_template}_parser.o section p4tc/parse "
            "action bpf obj {introspection}/"
            f"{self.base_template}_control_blocks.o section p4tc/main"
        )
        return cmd

    def spawn_bridge(self):
        return self.bridge.spawn_bridge()

    def boot(self):
        return self.virtme.boot()

    def _load_rules(self, rules_file):
        rules_file_basename = os.path.basename(rules_file)

        return self.virtme.run_intros("{introspection}/" + f"{rules_file_basename}")

    def load_rules(self, rules_file):
        if rules_file and os.path.isfile(rules_file):
            return self._load_rules(rules_file)

        return testutils.SUCCESS

    def parse_stf_files(self, stffile):
        """Parses the stf file and creates a .pcap file with input packets.
        It also adds the expected output packets per interface to a global
        dictionary.
        After parsing the necessary information, it sends the packets"""
        pipe_name, _ = os.path.splitext(os.path.basename(stffile))
        rules_file = None
        with open(stffile) as raw_stf:
            input_pkts, cmds, self.expected = parse_stf_file(raw_stf, pipe_name)
            if cmds:
                rules_file = f"{self.outputdir}/{self.base_template}.rules"
                json_file = f"{self.outputdir}/{self.base_template}.json"
                build_runtime_file(rules_file, json_file, cmds, self.virtme.ns.port_mapping)
        return input_pkts, rules_file

    def _init_tcpdump_listeners(self):
        # Listen to packets with tcpdump on all the ports of the ns
        pids = []
        for i, port in enumerate(self.virtme.port_mapping):
            outfile_name = self.filename(self.virtme.ns.mnt_dir, i, "out")
            pid = self.virtme.run_tcpdump(outfile_name, port)
            if pid == -1:
                return []
            pids.append(pid)

        # Wait for tcpdump to initialise
        time.sleep(2)

        return pids

    def send_packets(self, input_pkts):
        return self._send_pcap_files(input_pkts)

    def _load_extern_mods(self, extern_mods):
        for extern_mod, _ in extern_mods.items():
            self.virtme.run(f"modprobe {extern_mod}")

    def run(self, testfile, extern_mods):
        # Root is necessary to load ebpf into the kernel
        if not testutils.check_root():
            testutils.log.warning("This test requires root privileges; skipping execution.")
            return testutils.SKIPPED

        # Create the namespace and the central testing bridge
        result = self.virtme.ns_init()
        if result != testutils.SUCCESS:
            return result

        self._load_extern_mods(extern_mods)
        # Load template script
        self.base_template = os.path.basename(self.template)
        result = self.virtme.run_intros("{introspection}/" + f"{self.base_template}.template")
        if result != testutils.SUCCESS:
            return result

        input_pkts, rules_file = self.parse_stf_files(testfile)

        result = self.load_rules(rules_file)
        if result != testutils.SUCCESS:
            return result

        result = self.virtme.run_intros(self.load_filter_cmd(self.virtme.ns))
        if result != testutils.SUCCESS:
            return result

        pids = self._init_tcpdump_listeners()

        result = self.send_packets(input_pkts)
        if result != testutils.SUCCESS:
            return result

        time.sleep(2)

        result = self._kill_processes(pids)

        time.sleep(1)

        return result

    def check_outputs(self):
        """Checks if the output of the filter matches expectations"""
        testutils.log.info("Comparing outputs")
        direction = "out"
        for file in glob(self.filename(self.runtimedir, "*", direction)):
            testutils.log.info("Checking file %s", file)
            interface = self.interface_of_filename(file)
            if os.stat(file).st_size == 0:
                packets = []
            else:
                try:
                    packets = rdpcap(file)
                except Exception as e:
                    testutils.log.error("Corrupt pcap file %s\n%s", file, e)
                    return testutils.FAILURE

            if interface not in self.expected:
                expected = []
            else:
                # Check for expected packets.
                if self.expected[interface]["any"]:
                    if self.expected[interface]["pkts"]:
                        testutils.log.error(
                            (
                                "Interface %s has both expected with packets and without",
                                interface,
                            )
                        )
                    continue
                expected = self.expected[interface]["pkts"]
            if len(expected) != len(packets):
                testutils.log.error(
                    "Expected %s packets on port %s got %s",
                    len(expected),
                    interface,
                    len(packets),
                )
                return testutils.FAILURE
            for idx, expected_pkt in enumerate(expected):
                cmp = testutils.compare_pkt(expected_pkt, bytes(packets[idx]))
                if cmp != testutils.SUCCESS:
                    testutils.log.error("Packet %s on port %s differs", idx, interface)
                    return cmp
            # Remove successfully checked interfaces
            if interface in self.expected:
                del self.expected[interface]
        if len(self.expected) != 0:
            # Didn't find all the expects we were expecting
            testutils.log.error("Expected packets on port(s) %s not received", self.expected.keys())
            return testutils.FAILURE
        testutils.log.info("All went well.")
        return testutils.SUCCESS

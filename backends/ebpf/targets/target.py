#!/usr/bin/env python3
# Copyright 2013-present Barefoot Networks, Inc.
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
""" Contains different eBPF models and specifies their individual behavior
    Currently five phases are defined:
   1. Invokes the specified compiler on a provided p4 file.
   2. Parses an stf file and generates an pcap output.
   3. Loads the generated template or compiles it to a runnable binary.
   4. Feeds the generated pcap test packets into the P4 "filter"
   5. Evaluates the output with the expected result from the .stf file
"""

import os
import sys
from glob import glob
from pathlib import Path

import scapy.utils as scapy_util

from .ebpfstf import create_table_file, parse_stf_file

# path to the tools folder of the compiler
# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../../tools")))
import testutils

PCAP_PREFIX = "pcap"  # match pattern
PCAP_SUFFIX = ".pcap"  # could also be ".pcapng"


class EBPFTarget:
    """Parent Object of the EBPF Targets
    Defines common functions and variables"""

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

    def get_make_args(self, runtimedir, target):
        args = "make "
        # target makefile
        args += "-f runtime.mk "
        # Source folder of the makefile
        args += f"-C {runtimedir} "
        args += f"TARGET={target} "
        return args

    def filename(self, interface, direction):
        """Constructs the pcap filename from the given interface and
        packet stream direction. For example "pcap1_out.pcap" implies
        that the given stream contains tx packets from interface 1"""
        return self.tmpdir + "/" + PCAP_PREFIX + str(interface) + "_" + direction + PCAP_SUFFIX

    def interface_of_filename(self, f):
        """Extracts the interface name out of a pcap filename"""
        return int(os.path.basename(f).rstrip(PCAP_SUFFIX).lstrip(PCAP_PREFIX).rsplit("_", 1)[0])

    def compile_p4(self, argv):
        # To override
        """Compile the p4 target"""
        if not os.path.isfile(self.options.p4filename):
            testutils.log.error(f"No such file {self.options.p4filename}")
            sys.exit(testutils.FAILURE)
        # Initialize arguments for the makefile
        args = self.get_make_args(self.runtimedir, self.options.target)
        # name of the makefile target
        args += self.template + ".c "
        # name of the output source file
        args += f"BPFOBJ={self.template}.c "
        # location of the P4 input file
        args += f"P4FILE={self.options.p4filename} "
        # location of the P4 compiler
        args += f"P4C={self.compiler}"
        p4_args = " ".join(map(str, argv))
        if p4_args:
            # Remaining arguments
            args += f' P4ARGS="{p4_args}" '
        out, returncode = testutils.exec_process(args)
        if returncode != testutils.SUCCESS:
            testutils.log.error("Failed to compile the P4 program.")
            # If the compiler crashed fail the test
            if "Compiler Bug" in out:
                sys.exit(testutils.FAILURE)

        # Check if we expect the p4 compilation of the p4 file to fail
        expected_error = testutils.is_err(self.options.p4filename)
        if expected_error:
            # We do, so invert the result
            if returncode == testutils.SUCCESS:
                returncode = testutils.FAILURE
            else:
                returncode = testutils.SUCCESS
        return returncode, expected_error

    def _write_pcap_files(self, iface_pkts_map):
        """Writes the collected packets to their respective interfaces.
        This is done by creating a pcap file with the corresponding name."""
        for iface, pkts in iface_pkts_map.items():
            direction = "in"
            infile = self.filename(iface, direction)
            # Linktype 1 the Ethernet Link Type, see also 'man pcap-linktype'
            fp = scapy_util.RawPcapWriter(infile, linktype=1)
            fp._write_header(None)
            for pkt_data in pkts:
                try:
                    fp._write_packet(pkt_data)
                except ValueError:
                    testutils.log.error(f"Invalid packet data {pkt_data}")
                    return testutils.FAILURE
            fp.flush()
            fp.close()
            # debug -- the bytes in the pcap file should be identical to the
            # hex strings in the STF file
            # packets = rdpcap(infile)
            # for p in packets:
            #     print(p.convert_to(Raw).show())

        return testutils.SUCCESS

    def generate_model_inputs(self, stffile):
        # To override
        """Parses the stf file and creates a .pcap file with input packets.
        It also adds the expected output packets per interface to a global
        dictionary.
        After parsing the necessary information, it creates a control
        header for the runtime, which contains the extracted control
         plane commands"""
        with open(stffile) as raw_stf:
            input_pkts, cmds, self.expected = parse_stf_file(raw_stf)
            result, _ = create_table_file(cmds, self.tmpdir, "control.h")
            if result != testutils.SUCCESS:
                return result
            result = self._write_pcap_files(input_pkts)
            if result != testutils.SUCCESS:
                return result
        return testutils.SUCCESS

    def compile_dataplane(self, argv=""):
        # To override
        """Compiles a filter from the previously generated template"""
        raise NotImplementedError("Method create_filter not implemented!")

    def run(self, argv=""):
        # To override
        """Runs the filter and feeds attached interfaces with packets"""
        raise NotImplementedError("Method run() not implemented!")

    def check_outputs(self):
        """Checks if the output of the filter matches expectations"""
        testutils.log.info("Comparing outputs")
        direction = "out"
        for file in glob(self.filename("*", direction)):
            testutils.log.info("Checking file %s", file)
            interface = self.interface_of_filename(file)
            if os.stat(file).st_size == 0:
                packets = []
            else:
                try:
                    packets = scapy_util.rdpcap(file)
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
                cmp = testutils.compare_pkt(expected_pkt, packets[idx])
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

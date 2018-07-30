#!/usr/bin/env python
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
from scapy.utils import rdpcap
from scapy.layers.all import RawPcapWriter
from ebpfstf import create_table_file, parse_stf_file
from ebpfenv import Bridge
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../../tools')
from testutils import *


class EBPFFactory(object):
    """ Generator class.
     Returns a target subclass based on the provided target option."""
    @staticmethod
    def create(tmpdir, options, template, outputs):
        if options.target == "kernel":
            return EBPFKernelTarget(tmpdir, options, template, outputs)
        if options.target == "bcc":
            return EBPFBCCTarget(tmpdir, options, template, outputs)
        if options.target == "test":
            return EBPFTestTarget(tmpdir, options, template, outputs)


class EBPFTarget(object):
    """ Parent Object of the EBPF Targets
     Defines common functions and variables"""

    def __init__(self, tmpdir, options, template, outputs):
        self.tmpdir = tmpdir        # dir in which all files are stored
        self.options = options      # contains meta information
        self.template = template    # template to generate a filter
        self.outputs = outputs      # contains standard and error output
        self.pcapPrefix = "pcap"    # could also be "pcapng"
        self.expected = {}          # expected packets per interface
        self.expectedAny = []       # num packets does not matter
        self.ebpfdir = os.path.dirname(__file__) + "/runtime"

    def get_make_args(self, ebpfdir, target):
        args = "make "
        # target makefile
        args += "-f runtime.mk "
        # Source folder of the makefile
        args += "-C " + ebpfdir + " "
        args += "TARGET=" + target + " "
        return args

    def filename(self, interface, direction):
        """ Constructs the pcap filename from the given interface and
            packet stream direction. For example "pcap1_out.pcap" implies
            that the given stream contains tx packets from interface 1 """
        return (self.tmpdir + "/" + self.pcapPrefix +
                str(interface) + "_" + direction + ".pcap")

    def interface_of_filename(self, f):
        """ Extracts the interface name out of a pcap filename"""
        return int(os.path.basename(f).rstrip('.pcap').
                   lstrip(self.pcapPrefix).rsplit('_', 1)[0])

    def compile_p4(self, argv):
        # To override
        """ Compile the p4 target """
        if not os.path.isfile(self.options.p4filename):
            raise Exception("No such file " + self.options.p4filename)
        # Initialize arguments for the makefile
        args = self.get_make_args(self.ebpfdir, self.options.target)
        # name of the makefile target
        args += self.template + ".c "
        # name of the output source file
        args += "BPFOBJ=" + self.template + ".c "
        # location of the P4 input file
        args += "P4FILE=" + self.options.p4filename + " "
        # location of the P4 compiler
        args += "P4C=" + self.options.compilerSrcDir + "/build/p4c-ebpf "
        p4_args = ' '.join(map(str, argv))
        if (p4_args):
            # Remaining arguments
            args += "P4ARGS=\"" + p4_args + "\" "
        errmsg = "Failed to compile P4:"
        result = run_timeout(self.options.verbose, args, TIMEOUT,
                             self.outputs, errmsg)
        if result != SUCCESS:
            # If the compiler crashed fail the test
            if 'Compiler Bug' in open(self.outputs["stderr"]).readlines():
                sys.exit(FAILURE)

        # Check if we expect the p4 compilation of the p4 file to fail
        expected_error = is_err(self.options.p4filename)
        if expected_error:
            # We do, so invert the result
            if result == SUCCESS:
                result = FAILURE
            else:
                result = SUCCESS
        return result, expected_error

    def _write_pcap_files(self, iface_pkts_map):
        """ Writes the collected packets to their respective interfaces.
        This is done by creating a pcap file with the corresponding name. """
        for iface, pkts in iface_pkts_map.items():
            infile = self.tmpdir + "/pcap%s_in.pcap" % iface
            fp = RawPcapWriter(infile, linktype=1)
            fp._write_header(None)
            for pkt_data in pkts:
                try:
                    fp._write_packet(pkt_data)
                except ValueError:
                    report_err(self.outputs["stderr"],
                               "Invalid packet data", pkt_data)
                    sys.exit(FAILURE)

    def generate_model_inputs(self, stffile):
        # To override
        """ Parses the stf file and creates a .pcap file with input packets.
            It also adds the expected output packets per interface to a global
            dictionary.
            After parsing the necessary information, it creates a control
            header for the runtime, which contains the extracted control
             plane commands """
        with open(stffile) as raw_stf:
            iface_pkts, cmds, self.expected, self.expectedAny = parse_stf_file(
                raw_stf)
            create_table_file(cmds, self.tmpdir, "control.h")
            self._write_pcap_files(iface_pkts)
        return SUCCESS

    def create_filter(self):
        # To override
        """ Compiles a filter from the previously generated template """
        raise NotImplementedError("Method create_filter not implemented!")

    def run(self):
        # To override
        """ Runs the filter and feeds attached interfaces with packets """
        raise NotImplementedError("Method run() not implemented!")

    def check_outputs(self):
        """ Checks if the output of the filter matches expectations """
        report_output(self.outputs["stdout"],
                      self.options.verbose, "Comparing outputs")
        direction = "out"
        for file in glob(self.filename('*', direction)):
            interface = self.interface_of_filename(file)
            if os.stat(file).st_size == 0:
                packets = []
            else:
                try:
                    packets = rdpcap(file)
                except Exception as e:
                    report_err(self.outputs["stderr"],
                               "Corrupt pcap file", file, e)
                    self.showLog()
                    return FAILURE

            # Check for expected packets.
            if interface in self.expectedAny:
                if interface in self.expected:
                    report_err(self.outputs["stderr"],
                               ("Interface " + interface +
                                " has both expected with packets and without"))
                continue
            if interface not in self.expected:
                expected = []
            else:
                expected = self.expected[interface]
            if len(expected) != len(packets):
                report_err(self.outputs["stderr"], "Expected", len(
                    expected), "packets on port",
                    str(interface), "got", len(packets))
                return FAILURE
            for i in range(0, len(expected)):
                cmp = compare_pkt(
                    self.outputs, expected[i], packets[i])
                if cmp != SUCCESS:
                    report_err(self.outputs["stderr"], "Packet", i, "on port",
                               str(interface), "differs")
                    return FAILURE
            # Remove successfully checked interfaces
            if interface in self.expected:
                del self.expected[interface]
        if len(self.expected) != 0:
            # Didn't find all the expects we were expecting
            report_err(self.outputs["stderr"], "Expected packects on ports",
                       self.expected.keys(), "not received")
            return FAILURE
        return SUCCESS


class EBPFKernelTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def create_filter(self):
        # Use clang to compile the generated C code to a LLVM IR
        args = "make "
        # target makefile
        args += "-f " + self.options.target + ".mk "
        # Source folder of the makefile
        args += "-C " + self.ebpfdir + " "
        # Input eBPF byte code
        args += self.template + ".o "
        # The bpf program to attach to the interface
        args += "BPFOBJ=" + self.template + ".o"
        errmsg = "Failed to compile the eBPF byte code:"
        return run_timeout(self.options.verbose, args, TIMEOUT,
                           self.outputs, errmsg)

    def _create_runtime(self):
        args = self.get_make_args(self.ebpfdir, self.options.target)
        # List of bpf programs to attach to the interface
        args += "BPFOBJ=" + self.template + " "
        args += "CFLAGS+=-DCONTROL_PLANE "
        args += "SOURCES="
        errmsg = "Failed to build the filter:"
        return run_timeout(self.options.verbose, args, TIMEOUT,
                           self.outputs, errmsg)

    def _create_bridge(self):
        # The namespace is the id of the process
        namespace = str(os.getpid())
        # Create the namespace and the bridge with all its ports
        br = Bridge(namespace, self.outputs, self.options.verbose)
        result = br.create_virtual_env(len(self.expected))
        if result != SUCCESS:
            br.ns_del()
            return None
        return br

    def _get_run_cmd(self):
        direction = "in"
        pcap_pattern = self.filename('', direction)
        num_files = len(glob(self.filename('*', direction)))
        report_output(self.outputs["stdout"],
                      self.options.verbose,
                      "Input file: %s" % pcap_pattern)
        # Main executable
        cmd = self.template + " "
        # Input pcap pattern
        cmd += "-f " + pcap_pattern + " "
        # Number of input interfaces
        cmd += "-n " + str(num_files) + " "
        # Debug flag (verbose output)
        cmd += "-d"
        return cmd

    def _load_tc_cmd(self, bridge, proc, port_name):
        # Load the specified eBPF object to "port_name" ingress and egress
        # As a side-effect, this may create maps in /sys/fs/bpf/tc/globals
        cmd_ingress = ("tc filter add dev %s ingress"
                       " bpf da obj %s section prog "
                       "verbose" % (port_name, self.template + ".o"))
        cmd_egress = ("tc filter add dev %s egress"
                      " bpf da obj %s section prog "
                      "verbose" % (port_name, self.template + ".o"))
        result = bridge.ns_proc_write(proc, cmd_ingress)
        if result != SUCCESS:
            return result
        return bridge.ns_proc_append(proc, cmd_egress)

    def _run_in_namespace(self, bridge):
        # Open a process in the new namespace
        proc = bridge.ns_proc_open()
        if not proc:
            return FAILURE
        # Get the command to load eBPF code to all the attached ports
        if len(bridge.br_ports) > 0:
            for port in bridge.br_ports:
                result = self._load_tc_cmd(bridge, proc, port)
                bridge.ns_proc_append(proc, "")
        else:
            # No ports attached (no pcap files), load to bridge instead
            result = self._load_tc_cmd(bridge, proc, bridge.br_name)
            bridge.ns_proc_append(proc, "")

        if result != SUCCESS:
            return result
        # Check if eBPF maps have actually been created
        result = bridge.ns_proc_write(proc,
                                      "ls -1 /sys/fs/bpf/tc/globals")
        if result != SUCCESS:
            return result
        # Finally, append the actual runtime command to the process
        result = bridge.ns_proc_append(proc, self._get_run_cmd())
        if result != SUCCESS:
            return result
        # Execute the command queue and close the process, retrieve result
        return bridge.ns_proc_close(proc)

    def run(self):
        # Root is necessary to load ebpf into the kernel
        require_root(self.outputs)
        result = self._create_runtime()
        if result != SUCCESS:
            return result
        # Create the namespace and the central testing bridge
        bridge = self._create_bridge()
        if not bridge:
            return FAILURE
        # Run the program in the generated namespace
        result = self._run_in_namespace(bridge)
        bridge.ns_del()
        return result


class EBPFBCCTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def create_filter(self):
        # Not implemented yet, just pass the test
        return SUCCESS

    def check_outputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS

    def run(self):
        # Not implemented yet, just pass the test
        return SUCCESS


class EBPFTestTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def create_filter(self):
        args = self.get_make_args(self.ebpfdir, self.options.target)
        # List of bpf programs to attach to the interface
        args += "BPFOBJ=" + self.template + " "
        args += "CFLAGS+=-DCONTROL_PLANE "
        errmsg = "Failed to build the filter:"
        return run_timeout(self.options.verbose, args, TIMEOUT,
                           self.outputs, errmsg)

    def run(self):
        report_output(self.outputs["stdout"],
                      self.options.verbose, "Running model")
        direction = "in"
        pcap_pattern = self.filename('', direction)
        num_files = len(glob(self.filename('*', direction)))
        report_output(self.outputs["stdout"],
                      self.options.verbose,
                      "Input file: %s" % pcap_pattern)
        # Main executable
        args = self.template + " "
        # Input pcap pattern
        args += "-f " + pcap_pattern + " "
        # Number of input interfaces
        args += "-n " + str(num_files) + " "
        # Debug flag (verbose output)
        args += "-d"
        errmsg = "Failed to execute the filter:"
        result = run_timeout(self.options.verbose, args,
                             TIMEOUT, self.outputs, errmsg)
        return result

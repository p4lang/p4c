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

# Contains different eBPF models and specifies their individual behavior
# Currently five phases are defined:
#   1. Invokes the specified compiler on a provided p4 file.
#   2. Parses an stf file and generates an pcap output.
#   3. Loads the generated template or compiles it to a runnable binary.
#   4. Feeds the generated pcap test packets into the P4 "filter"
#   5. Evaluates the output with the expected result from the .stf file

from __future__ import print_function
import os
import sys
import subprocess
from subprocess import Popen
from threading import Timer
from glob import glob
from scapy.layers.all import RawPcapWriter
from scapy.utils import rdpcap
from pyroute2 import IPRoute, NetlinkError

SUCCESS = 0
FAILURE = 1
TIMEOUT = 10 * 60


def next_word(text, sep=None):
    """ Split a text at the indicated separator.
     Note that the separator can be a string.
     Separator is discarded. """
    spl = text.split(sep, 1)
    if len(spl) == 0:
        return '', ''
    elif len(spl) == 1:
        return spl[0].strip(), ''
    else:
        return spl[0].strip(), spl[1].strip()


def byte_to_hex(byteStr):
    return ''.join(["%02X " % ord(x) for x in byteStr]).strip()


def hex_to_byte(hexStr):
    bytes = []
    hexStr = ''.join(hexStr.split(" "))
    for i in range(0, len(hexStr), 2):
        bytes.append(chr(int(hexStr[i:i + 2], 16)))
    return ''.join(bytes)


def is_err(p4filename):
    """ True if the filename represents a p4 program that should fail. """
    return "_errors" in p4filename


def report_err(*message):
    print("***", file=sys.stderr, *message)


def run_timeout(options, args, timeout, stderr, errmsg):
    if options.verbose:
        print("Executing ", " ".join(args))
    proc = None
    errreport = ""

    def kill(process):
        process.kill()

    procstderr = None
    if stderr is not None:
        procstderr = open(stderr, "w")
        try:
            proc = Popen(args, stdout=subprocess.PIPE, stderr=procstderr)
        except OSError as e:
            print("Failed executing: ", e)
    if proc is None:
        # Never even started
        if options.verbose:
            print("Process failed to start")
        return FAILURE

    timer = Timer(TIMEOUT, kill, [proc])
    try:
        timer.start()
        out, err = proc.communicate()
    finally:
        timer.cancel()

    if options.verbose:
        print ("\n########### PROCESS OUTPUT BEGIN:\n", out)
        print ("########### PROCESS OUTPUT END\n")
    if proc.returncode != SUCCESS:
        procstderr = open(stderr, "r+")
        errreport = procstderr.read()
        procstderr.seek(0, 0)
        procstderr.write("Error %d: %s\n%s" %
                         (proc.returncode, errmsg, errreport))
        procstderr.close
        report_err("Error %d: %s\n%s" %
                   (proc.returncode, errmsg, errreport))
    return proc.returncode, errreport


def compare_pkt(expected, received):
    received = ''.join(byte_to_hex(str(received)).split()).upper()
    expected = ''.join(expected.split()).upper()
    if len(received) < len(expected):
        report_err("Received packet too short",
                   len(received), "vs", len(expected))
        return FAILURE
    for i in range(0, len(expected)):
        if expected[i] == "*":
            continue
        if expected[i] != received[i]:
            report_err("Received packet ", received)
            report_err("Packet different at position", i,
                       ": expected", expected[i], ", received", received[i])
            report_err("Full received packed is ", received)
            return FAILURE
    return SUCCESS


def get_make_args(ebpfdir, target):
    args = ["make"]
    # target makefile
    args.extend(["-f", target + ".mk"])
    # Source folder of the makefile
    args.extend(["-C", ebpfdir])
    return args


class EBPFFactory(object):
    """ Generator class.
     Returns a target subclass based on the provided target option."""
    @staticmethod
    def create(tmpdir, options, template, stderr):
        if options.target == "kernel":
            return EBPFKernelTarget(tmpdir, options, template, stderr)
        if options.target == "bcc":
            return EBPFBCCTarget(tmpdir, options, template, stderr)
        if options.target == "test":
            return EBPFTestTarget(tmpdir, options, template, stderr)


class EBPFTarget(object):
    """ Parent Object of the EBPF Targets
     Defines common functions and variables"""

    def __init__(self, tmpdir, options, template, stderr):
        self.tmpdir = tmpdir        # dir in which all files are stored
        self.options = options      # contains meta information
        self.template = template    # template to generate a filter
        self.stderr = stderr        # error file
        self.pcapPrefix = "pcap"    # could also be "pcapng"
        self.interfaces = {}        # list of active interfaces
        self.expected = {}          # expected packets per interface
        self.expectedAny = []       # num packets does not matter

    def filename(self, interface, direction):
        """ Constructs the pcap filename from the given interface and
            packet stream direction. For example "pcap1_out.pcap" implies
            that the given stream contains tx packets from interface 1 """
        return (self.tmpdir + "/" + self.pcapPrefix +
                str(interface) + "_" + direction + ".pcap")

    def interface_of_filename(self, f):
        return int(os.path.basename(f).rstrip('.pcap').
                   lstrip(self.pcapPrefix).rsplit('_', 1)[0])

    def compile_p4(self, argv):
        """ To override """
        """ Compile the p4 target """
        if not os.path.isfile(self.options.p4filename):
            raise Exception("No such file " + self.options.p4filename)
        args = ["./p4c-ebpf"]
        args.extend(["--target", self.options.target])
        args.extend(["-o", self.template])
        args.extend(argv)
        result, errtext = run_timeout(
            self.options, args, TIMEOUT, self.stderr, "Failed to compile P4:")
        if result != SUCCESS:
            print("".join(open(self.stderr).readlines()))
            # If the compiler crashed fail the test
            if 'Compiler Bug' in open(self.stderr).readlines():
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

    def generate_model_inputs(self, stffile):
        """ To override """
        """ Parses the stf file and creates a .pcap file with input packets.
            It also adds the expected output packets per interface to a global
            dictionary.
            TODO: Create pcap packet data structure and differentiate
            between input interfaces."""
        infile = self.tmpdir + "/in.pcap"
        fp = RawPcapWriter(infile, linktype=1)
        fp._write_header(None)
        with open(stffile) as i:
            for line in i:
                line, comment = next_word(line, "#")
                first, cmd = next_word(line)
                if first == "packet":
                    interface, cmd = next_word(cmd)
                    interface = int(interface)
                    data = ''.join(cmd.split())
                    try:
                        fp._write_packet(hex_to_byte(data))
                    except ValueError:
                        report_err("Invalid packet data", data)
                        os.remove(infile)
                        return FAILURE
                elif first == "expect":
                    interface, data = next_word(cmd)
                    interface = int(interface)
                    data = ''.join(data.split())
                    if data != '':
                        self.expected.setdefault(interface, []).append(data)
                    else:
                        self.expectedAny.append(interface)
        return SUCCESS

    def create_filter(self):
        """ To override """
        """ Compiles a filter from the previously generated template """
        return SUCCESS

    def run(self):
        """ To override """
        """ Runs the filter and feeds attached interfaces with packets """
        return SUCCESS

    def check_outputs(self):
        """ Checks if the output of the filter matches expectations """
        if self.options.verbose:
            print("Comparing outputs")
        direction = "out"
        for file in glob(self.filename('*', direction)):
            interface = self.interface_of_filename(file)
            if os.stat(file).st_size == 0:
                packets = []
            else:
                try:
                    packets = rdpcap(file)
                except:
                    report_err("Corrupt pcap file", file)
                    self.showLog()
                    return FAILURE

            # Check for expected packets.
            if interface in self.expectedAny:
                if interface in self.expected:
                    report_err("Interface " + interface +
                               " has both expected with packets and without")
                continue
            if interface not in self.expected:
                expected = []
            else:
                expected = self.expected[interface]
            if len(expected) != len(packets):
                report_err("Expected", len(expected), "packets on port",
                           str(interface), "got", len(packets))
                return FAILURE
            for i in range(0, len(expected)):
                cmp = compare_pkt(expected[i], packets[i])
                if cmp != SUCCESS:
                    report_err("Packet", i, "on port",
                               str(interface), "differs")
                    return FAILURE
            # Remove successfully checked interfaces
            if interface in self.expected:
                del self.expected[interface]
        if len(self.expected) != 0:
            # Didn't find all the expects we were expecting
            report_err("Expected packects on ports",
                       self.expected.keys(), "not received")
            return FAILURE
        else:
            return SUCCESS


class EBPFKernelTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, stderr):
        EBPFTarget.__init__(self, tmpdir, options, template, stderr)

    def compile_p4(self, argv):
        """ To override """
        """ Compile the p4 target """
        if not os.path.isfile(self.options.p4filename):
            raise Exception("No such file " + self.options.p4filename)
        ebpfdir = os.path.dirname(__file__)
        # Initialize arguments for the makefile
        args = get_make_args(ebpfdir, self.options.target)
        # name of the output source file
        args.append("BPFOBJ=" + self.template)
        # location of the P4 input file
        args.append("P4FILE=" + self.options.p4filename)
        p4_args = ' '.join(map(str, argv))
        # Remaining arguments
        args.append("ARGS=\"" + p4_args + "\"")

        errmsg = "Failed to compile P4:"
        result, errtext = run_timeout(
            self.options, args, TIMEOUT, self.stderr, errmsg)

        if result != SUCCESS:
            print("".join(open(self.stderr).readlines()))
            # If the compiler crashed fail the test
            if 'Compiler Bug' in open(self.stderr).readlines():
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

    def _create_interface(self, ifname):
        ip = IPRoute()
        try:
            device = ip.link_lookup(ifname=ifname)
            if len(device):
                # Interface exists already
                if self.options.verbose:
                    print("Trying to replace existing dummy interface...")
                ip.link("remove", ifname=ifname, kind="dummy")
            ip.link("add", ifname=ifname, kind="dummy")
        except Exception as e:
            # Something broke, provide feedback
            procstderr = open(self.stderr, "a+")
            procstderr.write(e.message)
            procstderr.close()

    def _remove_interface(self, ifname):
        ip = IPRoute()
        device = ip.link_lookup(ifname=ifname)
        if len(device):
            # Interface actually exists
            ip.link("remove", ifname=ifname, kind="dummy")

    def _compile_and_load_ebpf(self, ebpfdir, ifname):
        args = get_make_args(ebpfdir, self.options.target)
        # Input eBPF byte code
        template_bc = os.path.splitext(self.template)[0] + ".o"
        args.append(template_bc)
        # List of bpf programs to attach to the interface
        args.append("BPFOBJ=" + template_bc)
        # Path of the temporary test dir
        args.append("WORKDIR=" + self.tmpdir)
        # Name of the interface
        args.append("IFACE=" + ifname)
        errmsg = "Failed to compile and load the eBPF byte code:"
        return run_timeout(self.options, args, TIMEOUT, self.stderr, errmsg)

    def create_filter(self):
        # Create interface with unique id (allows parallel tests)
        ifname = "test" + str(os.getpid())
        self._create_interface(ifname)
        ebpfdir = os.path.dirname(__file__)
        # Use clang to compile the generated C code to a LLVM IR
        result, errtext = self._compile_and_load_ebpf(ebpfdir, ifname)
        if result != SUCCESS:
            self._remove_interface(ifname)
            return result
        self._remove_interface(ifname)
        return result

    def check_outputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS


class EBPFBCCTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, stderr):
        EBPFTarget.__init__(self, tmpdir, options, template, stderr)

    def check_outputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS


class EBPFTestTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, stderr):
        EBPFTarget.__init__(self, tmpdir, options, template, stderr)

    def create_filter(self):
        ebpfdir = os.path.dirname(__file__)
        # compiler
        args = ["gcc"]
        args.append("-g")
        # main
        args.append("-o")
        args.append(self.tmpdir + "/filter")
        # sources
        args.append(ebpfdir + "/ebpf_runtime.c")
        args.append(ebpfdir + "/testinclude/ebpf_registry.c")
        args.append(ebpfdir + "/testinclude/ebpf_map.c")
        args.append(self.template)
        # includes
        args.append("-I" + self.tmpdir)
        args.append("-I" + ebpfdir)
        # libs
        args.append("-lpcap")
        errmsg = "Failed to build the filter:"
        result, errtext = run_timeout(
            self.options, args, TIMEOUT, self.stderr, errmsg)
        return result

    def run(self):
        if self.options.verbose:
            print("Running model")
        # main
        args = [self.tmpdir + "/filter"]
        # input
        args.append(self.tmpdir + "/in.pcap")
        errmsg = "Failed to execute the filter:"
        result, errtext = run_timeout(
            self.options, args, TIMEOUT, self.stderr, errmsg)
        return result

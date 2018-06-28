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
from pyroute2 import IPRoute
sys.path.insert(0, os.path.dirname(__file__) + '/../../tools/stf')
from stf_parser import STFParser

SUCCESS = 0
FAILURE = 1
TIMEOUT = 10 * 60


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


def report_err(file, *message):
    print("***", file=sys.stderr, *message)

    if (file and file != sys.stderr):
        err_file = open(file, "a+")
        print("***", file=err_file, *message)
        err_file.close()


def report_output(file, verbose, *message):
    if (verbose):
        print(file=sys.stdout, *message)

    if (file and file != sys.stdout):
        out_file = open(file, "a+")
        print("", file=out_file, *message)
        out_file.close()


def run_timeout(options, args, timeout, outputs, errmsg):
    report_output(outputs["stdout"],
                  options.verbose, "Executing ", " ".join(args))
    proc = None

    def kill(process):
        process.kill()

    if outputs["stderr"] is not None:
        try:
            proc = Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except OSError as e:
            report_err(outputs["stderr"], "Failed executing: ", e)
    if proc is None:
        # Never even started
        report_err(outputs["stderr"], "Process failed to start")
        return FAILURE

    timer = Timer(TIMEOUT, kill, [proc])
    try:
        timer.start()
        out, err = proc.communicate()
    finally:
        timer.cancel()
    msg = ("\n########### PROCESS OUTPUT BEGIN:\n"
           "%s########### PROCESS OUTPUT END\n" % out)
    report_output(outputs["stdout"], options.verbose, msg)
    if proc.returncode != SUCCESS:
        report_err(outputs["stderr"], "Error %d: %s\n%s" %
                   (proc.returncode, errmsg, err))
    return proc.returncode


def compare_pkt(outputs, expected, received):
    received = ''.join(byte_to_hex(str(received)).split()).upper()
    expected = ''.join(expected.split()).upper()
    if len(received) < len(expected):
        report_err(outputs["stderr"], "Received packet too short",
                   len(received), "vs", len(expected))
        return FAILURE
    for i in range(0, len(expected)):
        if expected[i] == "*":
            continue
        if expected[i] != received[i]:
            report_err(outputs["stderr"], "Received packet ", received)
            report_err(outputs["stderr"], "Packet different at position", i,
                       ": expected", expected[i], ", received", received[i])
            report_err(outputs["stderr"], "Full received packed is ", received)
            return FAILURE
    return SUCCESS


def require_root(outputs):
    if (not (os.getuid() == 0)):
        errmsg = "This test requires root privileges! Failing..."
        report_err(outputs["stderr"], errmsg)
        sys.exit(FAILURE)


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
        self.interfaces = {}        # list of active interfaces
        self.expected = {}          # expected packets per interface
        self.expectedAny = []       # num packets does not matter
        # TODO: Make the runtime dir independent
        #       on the location of the python file
        self.ebpfdir = os.path.dirname(__file__) + "/runtime"

    def get_make_args(self, ebpfdir, target):
        args = ["make"]
        # target makefile
        args.extend(["-f", target + ".mk"])
        # Source folder of the makefile
        args.extend(["-C", ebpfdir])
        return args

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
        # To override
        """ Compile the p4 target """
        if not os.path.isfile(self.options.p4filename):
            raise Exception("No such file " + self.options.p4filename)
        # Initialize arguments for the makefile
        args = self.get_make_args(self.ebpfdir, self.options.target)
        # name of the output source file
        args.append("BPFOBJ=" + self.template + ".c")
        # location of the P4 input file
        args.append("P4FILE=" + self.options.p4filename)
        # location of the P4 compiler
        args.append("P4C=" + self.options.compilerSrcDir + "/build/p4c-ebpf")
        p4_args = ' '.join(map(str, argv))
        if (p4_args):
            # Remaining arguments
            args.append("P4ARGS=\"" + p4_args + "\"")
        errmsg = "Failed to compile P4:"
        result = run_timeout(self.options, args, TIMEOUT,
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

    def _generate_control_actions(self, actions):
        """ Generates the actual control plane commands.
        This function inserts C code for all the "add" commands that have
        been parsed. """
        generated = ""
        for index, cmd in enumerate(actions):
            key_name = "key_%s%d" % (cmd["table"], index)
            value_name = "value_%s%d" % (cmd["table"], index)
            if cmd["type"] == "setdefault":
                tbl_name = cmd["table"] + "_defaultAction"
            else:
                tbl_name = cmd["table"]
            generated = "struct %s_key %s = {};\n\t" % (cmd["table"], key_name)
            for key_num, key_field in enumerate(cmd["match"]):
                field = key_field[0].split('.')[1]
                generated += ("%s.%s = %s;\n\t"
                              % (key_name, field, key_field[1]))
            generated += ("int tableFileDescriptor = "
                          "BPF_OBJ_GET(MAP_PATH \"/%s\");\n\t" %
                          tbl_name)
            generated += ("if (tableFileDescriptor < 0) {"
                          "fprintf(stderr, \"map %s not loaded\");"
                          " exit(1); }\n\t" % tbl_name)
            generated += ("struct %s_value %s = {};\n\t" % (
                cmd["table"], value_name))
            generated += "%s.action = %s;\n\t" % (value_name, cmd["action"][0])
            if cmd["action"][1]:
                generated += "%s.u = {" % value_name
                for val_num, val_field in enumerate(cmd["action"][1]):
                    generated += "%s," % val_field[1]
                generated += "};\n\t"
            generated += ("ok = BPF_USER_MAP_UPDATE_ELEM"
                          "(tableFileDescriptor, &%s, &%s, BPF_ANY);\n\t"
                          % (key_name, value_name))
            generated += ("if (ok != 0) { perror(\"Could not write in %s\");"
                          "exit(1); }\n" % tbl_name)
        return generated

    def _create_table_file(self, actions):
        """ Create the control plane file.
        The control commands are provided by the stf parser.
        This generated file is required by ebpf_runtime.c to initialize
        the control plane. """
        control_file = open(self.tmpdir + "/control.h", "w+")
        print (self.tmpdir + "/control.h")
        control_file.write("#include \"test.h\"\n\n")
        control_file.write("static inline void generated_init() {\n\t")
        control_file.write("int ok;\n\t")
        generated_cmds = self._generate_control_actions(actions)
        control_file.write(generated_cmds)
        control_file.write("}\n")

    def _parse_stf_file(self, raw_stf):
        """ Uses the .stf parsing tool to acquire a pre-formatted list.
            Processing entries according to their specified cmd. """
        parser = STFParser()
        stf_str = raw_stf.read()
        stf_map, errs = parser.parse(stf_str)
        iface_pkts = {}
        actions = []
        print(stf_map)
        for stf_entry in stf_map:
            if stf_entry[0] == "packet":
                iface_pkts.setdefault(stf_entry[1], []).append(
                    hex_to_byte(stf_entry[2]))
            elif stf_entry[0] == "expect":
                interface = int(stf_entry[1])
                pkt_data = stf_entry[2]
                if pkt_data != '':
                    self.expected.setdefault(
                        interface, []).append(pkt_data)
                else:
                    self.expectedAny.append(interface)
            elif stf_entry[0] == "add":
                cmd = {}
                cmd["type"] = stf_entry[0]
                cmd["table"] = stf_entry[1]
                cmd["priority"] = stf_entry[2]  # not supported
                cmd["match"] = stf_entry[3]
                cmd["action"] = stf_entry[4]
                cmd["whatisthis"] = stf_entry[5] # not supported
                actions.append(cmd)
            elif stf_entry[0] == "setdefault":
                cmd = {}
                cmd["type"] = stf_entry[0]
                cmd["table"] = stf_entry[1]
                cmd["match"] = "" # not necessary to match
                cmd["priority"] = ""  # not supported
                cmd["action"] = stf_entry[2]
                actions.append(cmd)
        return iface_pkts, actions

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
            iface_pkts, actions = self._parse_stf_file(raw_stf)
            self._write_pcap_files(iface_pkts)
            self._create_table_file(actions)
        return SUCCESS

    def create_filter(self):
        # To override
        """ Compiles a filter from the previously generated template """
        return SUCCESS

    def run(self):
        # To override
        """ Runs the filter and feeds attached interfaces with packets """
        return SUCCESS

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
        else:
            return SUCCESS


class EBPFKernelTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def _create_bridge(self, br_name):
        report_output(self.outputs["stdout"],
                      self.options.verbose, "Creating the bridge...")
        ipr = IPRoute()
        ipr.link_create(ifname=br_name, kind='bridge')
        for index in (range(len(self.expected))):
            if_bridge = "%s_%d" % (br_name, index)
            if_veth = "veth_%s_%d" % (br_name, index)
            print (if_bridge, if_veth)
            ipr.link_create(ifname=if_veth, kind="veth", peer=if_bridge)
            ipr.link('set', index=ipr.link_lookup(ifname=if_veth)[
                     0], master=ipr.link_lookup(ifname=br_name)[0])
        ipr.link("set", index=ipr.link_lookup(ifname=br_name), state="up")

    def _remove_bridge(self, br_name):
        report_output(self.outputs["stdout"],
                      self.options.verbose, "Deleting the bridge...")
        ipr = IPRoute()
        ipr.link_remove(index=ipr.link_lookup(ifname=br_name)[0])
        for index in (range(len(self.expected))):
            if_bridge = "%s_%d" % (br_name, index)
            ipr.link_remove(index=ipr.link_lookup(ifname=if_bridge)[0])

    def _compile_ebpf(self, ebpfdir):
        args = self.get_make_args(ebpfdir, self.options.target)
        # Input eBPF byte code
        args.append(self.template + ".o")
        # The bpf program to attach to the interface
        args.append("BPFOBJ=" + self.template + ".o")
        errmsg = "Failed to compile the eBPF byte code:"
        return run_timeout(self.options, args, TIMEOUT, self.outputs, errmsg)

    def _load_ebpf(self, ebpfdir, ifname):
        args = ["tc"]
        # Create a qdisc for our custom virtual interface
        args.extend(["qdisc", "add", "dev", ifname, "clsact"])
        errmsg = "Failed to add tc qdisc:"
        result = run_timeout(self.options, args, TIMEOUT, self.outputs, errmsg)
        if result != SUCCESS:
            return result
        args = ["tc"]
        # Launch tc to load the ebpf object to the specified interface
        args.extend(["filter", "add", "dev", ifname, "ingress",
                     "bpf", "da", "obj", self.template + ".o",
                     "sec", "prog", "verb"])
        # The bpf program to attach to the interface
        errmsg = "Failed to load the eBPF byte code using tc:"
        return run_timeout(self.options, args, TIMEOUT, self.outputs, errmsg)

    def create_filter(self):
        require_root(self.outputs)
        # Create interface with unique id (allows parallel tests)
        # TODO: Create one custom interface per pcap file we parse
        ifname = str(os.getpid())
        self._create_bridge(ifname)
        # Use clang to compile the generated C code to a LLVM IR
        result = self._compile_ebpf(self.ebpfdir)
        if result != SUCCESS:
            self._remove_bridge(ifname)
            return result
        result = self._load_ebpf(self.ebpfdir, ifname)
        if result != SUCCESS:
            self._remove_bridge(ifname)
        return result

    def check_outputs(self):
        # Not implemented yet, just pass the test
        ifname = str(os.getpid())
        self._remove_bridge(ifname)
        return SUCCESS


class EBPFBCCTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def compile_p4(self, argv):
        # To override
        """ Compile the p4 target """
        if not os.path.isfile(self.options.p4filename):
            raise Exception("No such file " + self.options.p4filename)
        args = ["./p4c-ebpf"]
        args.extend(["--target", self.options.target])
        args.extend(["-o", self.template])
        args.append(self.options.p4filename)
        args.extend(argv)
        result = run_timeout(self.options, args, TIMEOUT,
                             self.outputs, "Failed to compile P4:")
        if result != SUCCESS:
            print("".join(open(self.outputs["stderr"]).readlines()))
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

    def check_outputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS


class EBPFTestTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)

    def create_filter(self):
        args = self.get_make_args(self.ebpfdir, self.options.target)
        # List of bpf programs to attach to the interface
        args.append("BPFOBJ=" + self.template)
        errmsg = "Failed to build the filter:"
        return run_timeout(self.options, args, TIMEOUT, self.outputs, errmsg)

    def run(self):
        report_output(self.outputs["stdout"],
                      self.options.verbose, "Running model")
        direction = "in"
        for file in glob(self.filename('*', direction)):
            # Main executable
            args = [self.template]
            # Input
            args.extend(["-f", file])
            # Debug flag
            args.append("-d")
            errmsg = "Failed to execute the filter:"
            result = run_timeout(self.options, args,
                                 TIMEOUT, self.outputs, errmsg)
            if result != SUCCESS:
                return FAILURE
        return SUCCESS

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
from subprocess import Popen
from threading import Thread
from glob import glob
try:
    from scapy.layers.all import *
    from scapy.utils import *
except ImportError:
    pass

SUCCESS = 0
FAILURE = 1
TIMEOUT = 10 * 60


def nextWord(text, sep=None):
    ''' Split a text at the indicated separator.
     Note that the separator can be a string.
     Separator is discarded. '''
    spl = text.split(sep, 1)
    if len(spl) == 0:
        return '', ''
    elif len(spl) == 1:
        return spl[0].strip(), ''
    else:
        return spl[0].strip(), spl[1].strip()


def ByteToHex(byteStr):
    return ''.join(["%02X " % ord(x) for x in byteStr]).strip()


def HexToByte(hexStr):
    bytes = []
    hexStr = ''.join(hexStr.split(" "))
    for i in range(0, len(hexStr), 2):
        bytes.append(chr(int(hexStr[i:i + 2], 16)))
    return ''.join(bytes)


def isError(p4filename):
    ''' True if the filename represents a p4 program that should fail. '''
    return "_errors" in p4filename


def reportError(*message):
    print("***", *message)


class Local(object):
    ''' Object to hold local vars accessable to nested functions '''
    pass


def run_timeout(options, args, timeout, stderr):
    if options.verbose:
        print("Executing ", " ".join(args))
    local = Local()
    local.process = None
    errtext = ""

    def target():
        procstderr = None
        if stderr is not None:
            procstderr = open(stderr, "w")
        local.process = Popen(args, stdout=subprocess.PIPE, stderr=procstderr)
        out, err = local.process.communicate()
        if options.verbose:
            print(out)
    thread = Thread(target=target)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        print("Timeout ", " ".join(args), file=sys.stderr)
        local.process.terminate()
        thread.join()
    if options.verbose:
        print("Exit code %d\n" % local.process.returncode)
    if local.process is None:
        # never even started
        if options.verbose:
            print("Process failed to start")
        return FAILURE

    if local.process.returncode != SUCCESS:
        procstderr = open(stderr, "r")
        errtext = procstderr.read()
        procstderr.close
    return local.process.returncode, errtext


def comparePacket(expected, received):
    received = ''.join(ByteToHex(str(received)).split()).upper()
    expected = ''.join(expected.split()).upper()
    if len(received) < len(expected):
        reportError("Received packet too short",
                    len(received), "vs", len(expected))
        return FAILURE
    for i in range(0, len(expected)):
        if expected[i] == "*":
            continue
        if expected[i] != received[i]:
            reportError("Received packet ", received)
            reportError("Packet different at position", i,
                        ": expected", expected[i], ", received", received[i])
            reportError("Full received packed is ", received)
            return FAILURE
    return SUCCESS


class EBPFFactory(object):
    ''' Generator class.
     Returns a target subclass based on the provided target option.'''
    @staticmethod
    def create(tmpdir, options, template, stderr):
        if options.target == "kernel":
            return EBPFKernelTarget(tmpdir, options, template, stderr)
        if options.target == "bcc":
            return EBPFBCCTarget(tmpdir, options, template, stderr)
        if options.target == "test":
            return EBPFTestTarget(tmpdir, options, template, stderr)


class EBPFTarget(object):
    ''' Parent Object of the EBPF Targets
     Defines common functions and variables'''

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
        return (self.tmpdir + "/" + self.pcapPrefix +
                str(interface) + "_" + direction + ".pcap")

    def interface_of_filename(self, f):
        return int(os.path.basename(f).rstrip('.pcap').
                   lstrip(self.pcapPrefix).rsplit('_', 1)[0])

    def compile_p4(self, argv):
        ''' To override '''
        ''' Compile the p4 target '''
        if not os.path.isfile(self.options.p4filename):
            raise Exception("No such file " + self.options.p4filename)
        args = ["./p4c-ebpf"]
        args.extend(["--target", self.options.target])
        args.extend(["-o", self.template])
        args.extend(argv)
        result, errtext = run_timeout(self.options, args, TIMEOUT, self.stderr)
        if result != SUCCESS:
            reportError("Error %d: Failed to compile P4." %
                        (result))
            print("".join(open(self.stderr).readlines()))
            # If the compiler crashed fail the test
            if 'Compiler Bug' in open(self.stderr).readlines():
                sys.exit(FAILURE)
        # check if we expect the p4 compilation of the p4 file to fail
        expected_error = isError(self.options.p4filename)
        if expected_error:
            # we do, so invert the result
            if result == SUCCESS:
                result = FAILURE
            else:
                result = SUCCESS
        return result, expected_error

    def generate_model_inputs(self, stffile):
        ''' To override '''
        ''' Parses the stf file and creates a .pcap file with input packets.
            It also adds the expected output packets per interface to a global
            dictionary.
            TODO: Create pcap packet data structure and differentiate
            between input interfaces.'''
        infile = self.tmpdir + "/in.pcap"
        fp = RawPcapWriter(infile, linktype=1)
        fp._write_header(None)
        with open(stffile) as i:
            for line in i:
                line, comment = nextWord(line, "#")
                first, cmd = nextWord(line)
                if first == "packet":
                    interface, cmd = nextWord(cmd)
                    interface = int(interface)
                    data = ''.join(cmd.split())
                    try:
                        fp._write_packet(HexToByte(data))
                    except ValueError:
                        reportError("Invalid packet data", data)
                        os.remove(infile)
                        return FAILURE
                elif first == "expect":
                    interface, data = nextWord(cmd)
                    interface = int(interface)
                    data = ''.join(data.split())
                    if data != '':
                        self.expected.setdefault(interface, []).append(data)
                    else:
                        self.expectedAny.append(interface)
        return SUCCESS

    def create_filter(self):
        ''' To override '''
        ''' Compiles a filter from the previously generated template '''
        return SUCCESS

    def run(self):
        ''' To override '''
        ''' Runs the filter and feed attached interfaces with packets '''
        return SUCCESS

    def checkOutputs(self):
        ''' Checks if the output of the filter matches expectations '''
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
                    reportError("Corrupt pcap file", file)
                    self.showLog()
                    return FAILURE

            # Check for expected packets.
            if interface in self.expectedAny:
                if interface in self.expected:
                    reportError("Interface " + interface +
                                " has both expected with packets and without")
                continue
            if interface not in self.expected:
                expected = []
            else:
                expected = self.expected[interface]
            if len(expected) != len(packets):
                reportError("Expected", len(expected), "packets on port",
                            str(interface), "got", len(packets))
                return FAILURE
            for i in range(0, len(expected)):
                cmp = comparePacket(expected[i], packets[i])
                if cmp != SUCCESS:
                    reportError("Packet", i, "on port",
                                str(interface), "differs")
                    return FAILURE
            # remove successfully checked interfaces
            if interface in self.expected:
                del self.expected[interface]
        if len(self.expected) != 0:
            # didn't find all the expects we were expecting
            reportError("Expected packects on ports",
                        self.expected.keys(), "not received")
            return FAILURE
        else:
            return SUCCESS


class EBPFKernelTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, stderr):
        EBPFTarget.__init__(self, tmpdir, options, template, stderr)

    def checkOutputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS


class EBPFBCCTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, stderr):
        EBPFTarget.__init__(self, tmpdir, options, template, stderr)

    def checkOutputs(self):
        # Not implemented yet, just pass the test
        return SUCCESS


class EBPFTestTarget(EBPFTarget):
    def __init__(self, tmpdir, options, template, stderr):
        EBPFTarget.__init__(self, tmpdir, options, template, stderr)

    def create_filter(self):
        ebpfdir = os.path.dirname(__file__)
        # compiler
        args = ["gcc"]
        # main
        args.append("-o")
        args.append(self.tmpdir + "/filter")
        # sources
        args.append(ebpfdir + "/ebpf_runtime.c")
        args.append(ebpfdir + "/bpfinclude/ebpf_registry.c")
        args.append(ebpfdir + "/bpfinclude/ebpf_map.c")
        args.append(self.template)
        # includes
        args.append("-I" + self.tmpdir)
        args.append("-I" + ebpfdir)
        # libs
        args.append("-lpcap")
        result, errtext = run_timeout(self.options, args, TIMEOUT, self.stderr)
        if result != SUCCESS:
            reportError("Error %d: Failed to build the filter:\n%s" %
                        (result, errtext))
        return result

    def run(self):
        if self.options.verbose:
            print("Running model")
        # compiler
        args = []
        # main
        args.append(self.tmpdir + "/filter")
        # input
        args.append(self.tmpdir + "/in.pcap")
        result, errtext = run_timeout(self.options, args, TIMEOUT, self.stderr)
        if result != SUCCESS:
            reportError("Error %d: Failed to run the filter:\n%s" %
                        (result, errtext))
        return result

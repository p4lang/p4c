#!/usr/bin/env python2
# Copyright 2013-present Barefoot Networks, Inc.
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

# Runs the BMv2 behavioral model simulator with input from an stf file

from __future__ import print_function
from subprocess import Popen
from threading import Thread
from glob import glob
import json
import sys
import re
import os
import stat
import tempfile
import shutil
import difflib
import subprocess
import signal
import time
import random
import errno
import socket
from string import maketrans
from collections import OrderedDict
try:
    from scapy.layers.all import *
    from scapy.utils import *
except ImportError:
    pass

SUCCESS = 0
FAILURE = 1

class TimeoutException(Exception): pass
def signal_handler(signum, frame):
    raise TimeoutException, "Timed out!"
signal.signal(signal.SIGALRM, signal_handler)

class Options(object):
    def __init__(self):
        self.binary = None
        self.verbose = False
        self.preserveTmp = False
        self.observationLog = None
        self.usePsa = False

def nextWord(text, sep = None):
    # Split a text at the indicated separator.
    # Note that the separator can be a string.
    # Separator is discarded.
    spl = text.split(sep, 1)
    if len(spl) == 0:
        return '', ''
    elif len(spl) == 1:
        return spl[0].strip(), ''
    else:
        return spl[0].strip(), spl[1].strip()

def ByteToHex(byteStr):
    return ''.join( [ "%02X " % ord( x ) for x in byteStr ] ).strip()

def HexToByte(hexStr):
    bytes = []
    hexStr = ''.join( hexStr.split(" ") )
    for i in range(0, len(hexStr), 2):
        bytes.append( chr( int (hexStr[i:i+2], 16 ) ) )
    return ''.join( bytes )

def convert_packet_bin2hexstr(pkt_bin):
    return ''.join(ByteToHex(str(pkt_bin)).split()).upper()

def convert_packet_stf2hexstr(pkt_stf_text):
    return ''.join(pkt_stf_text.split()).upper()

def reportError(*message):
    print("***", *message)

class Local(object):
    # object to hold local vars accessable to nested functions
    pass

def FindExe(dirname, exe):
    dir = os.getcwd()
    while len(dir) > 1:
        if os.path.isdir(os.path.join(dir, dirname)):
            rv = None
            rv_time = 0
            for dName, sdName, fList in os.walk(os.path.join(dir, dirname)):
                if exe in fList:
                    n=os.path.join(dName, exe)
                    if os.path.isfile(n) and os.access(n, os.X_OK):
                        n_time = os.path.getmtime(n)
                        if n_time > rv_time:
                            rv = n
                            rv_time = n_time
            if rv is not None:
                return rv
        dir = os.path.dirname(dir)
    return exe

def run_timeout(verbose, args, timeout, stderr):
    if verbose:
        print("Executing ", " ".join(args))
    local = Local()
    local.process = None
    def target():
        procstderr = None
        if stderr is not None:
            procstderr = open(stderr, "w")
        local.process = Popen(args, stderr=procstderr)
        local.process.wait()
    thread = Thread(target=target)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        print("Timeout ", " ".join(args), file=sys.stderr)
        local.process.terminate()
        thread.join()
    if local.process is None:
        # never even started
        reportError("Process failed to start")
        return -1
    if verbose:
        print("Exit code ", local.process.returncode)
    return local.process.returncode

timeout = 10 * 60

class ConcurrentInteger(object):
    # Generates exclusive integers in a range 0-max
    # in a way which is safe across multiple processes.
    # It uses a simple form of locking using folder names.
    # This is necessary because this script may be invoked
    # concurrently many times by make, and we need the many simulator instances
    # to use different port numbers.
    def __init__(self, folder, max):
        self.folder = folder
        self.max = max
    def lockName(self, value):
        return "lock_" + str(value)
    def release(self, value):
        os.rmdir(self.lockName(value))
    def generate(self):
        # try 10 times
        for i in range(0, 10):
            index = random.randint(0, self.max)
            file = self.lockName(index)
            try:
                os.makedirs(file)
                return index
            except:
                time.sleep(1)
                continue
        return None

class BMV2ActionArg(object):
    def __init__(self, name, width):
        # assert isinstance(name, str)
        # assert isinstance(width, int)
        self.name = name
        self.width = width

class TableKey(object):
    def __init__(self):
        self.fields = OrderedDict()
    def append(self, name, type):
        self.fields[name] = type
    def __str__(self):
        result = ""
        for f in self.fields.keys():
            if result != "":
                result += " "
            result += f + ":" + self.fields[f]
        return result

class TableKeyInstance(object):
    def __init__(self, tableKey):
        assert isinstance(tableKey, TableKey)
        self.values = {}
        self.key = tableKey
        for f,t in tableKey.fields.iteritems():
            if t == "ternary":
                self.values[f] = "0&&&0"
            elif t == "lpm":
                self.values[f] = "0/0"
            elif t == "exact":
                self.values[f] = "0"
            elif t == "valid":
                self.values[f] = "0"
            else:
                raise Exception("Unexpected key type " + t)
    def set(self, key, value):
        array = re.compile("(.*)\$([0-9]+)(.*)");
        m = array.match(key)
        if m:
            key = m.group(1) + "[" + m.group(2) + "]" + m.group(3)

        found = False
        if key in self.key.fields:
            found = True
        elif key + '$' in self.key.fields:
            key = key + '$'
            found = True
        elif key + '.$valid$' in self.key.fields:
            key = key + '.$valid$'
            found = True
        elif key.endswith(".valid"):
            alt = key[:-5] + "$valid$"
            if alt in self.key.fields:
                key = alt
                found = True
        if not found:
            for i in self.key.fields:
                if i.endswith("." + key) or i.endswith("." + key + "$"):
                    key = i
                    found = True
                elif key == "valid" and i.endswith(".$valid$"):
                    key = i
                    found = True
        if not found and key == "valid" and "$valid$" in self.key.fields:
            key = "$valid$"
            found = True
        if not found:
            print(self.key.fields)
            raise Exception("Unexpected key field " + key)
        if self.key.fields[key] == "ternary":
            self.values[key] = self.makeMask(value)
        elif self.key.fields[key] == "lpm":
            self.values[key] = self.makeLpm(value)
        else:
            self.values[key] = value
    def makeMask(self, value):
        # TODO -- we really need to know the size of the key to make the mask properly,
        # but to find that, we need to parse the headers and header_types from the json
        if value.startswith("0x"):
            mask = "F"
            value = value[2:]
            prefix = "0x"
        elif value.startswith("0b"):
            mask = "1"
            value = value[2:]
            prefix = "0b"
        elif value.startswith("0o"):
            mask = "7"
            value = value[2:]
            prefix = "0o"
        else:
            raise Exception("Decimal value "+value+" not supported for ternary key")
            return value
        values = "0123456789abcdefABCDEF*"
        replacements = (mask * 22) + "0"
        trans = maketrans(values, replacements)
        m = value.translate(trans)
        return prefix + value.replace("*", "0") + "&&&" + prefix + m
    def makeLpm(self, value):
        if value.find('/') >= 0:
            return value
        if value.startswith("0x"):
            bits_per_digit = 4
        elif value.startswith("0b"):
            bits_per_digit = 1
        elif value.startswith("0o"):
            bits_per_digit = 3
        else:
            value = "0x" + hex(int(value))
            bits_per_digit = 4
        digits = len(value) - 2 - value.count('*')
        return value.replace('*', '0') + "/" + str(digits*bits_per_digit)
    def __str__(self):
        result = ""
        for f in self.key.fields:
            if result != "":
                result += " "
            result += self.values[f]
        return result

class BMV2ActionArguments(object):
    def __init__(self, action):
        assert isinstance(action, BMV2Action)
        self.action = action
        self.values = {}
    def set(self, key, value):
        found = False
        for i in self.action.args:
            if key == i.name:
                found = True
        if not found:
            raise Exception("Unexpected action arg " + key)
        self.values[key] = value
    def __str__(self):
        result = ""
        for f in self.action.args:
            if result != "":
                result += " "
            result += self.values[f.name]
        return result
    def size(self):
        return len(self.action.args)

class BMV2Action(object):
    def __init__(self, jsonAction):
        self.name = jsonAction["name"]
        self.args = []
        for a in jsonAction["runtime_data"]:
            arg = BMV2ActionArg(a["name"], a["bitwidth"])
            self.args.append(arg)
    def __str__(self):
        return self.name
    def makeArgsInstance(self):
        return BMV2ActionArguments(self)

class BMV2Table(object):
    def __init__(self, jsonTable):
        self.match_type = jsonTable["match_type"]
        self.name = jsonTable["name"]
        self.key = TableKey()
        self.actions = {}
        for k in jsonTable["key"]:
            name = k["name"]
            if name is None:
                name = k["target"]
            if isinstance(name, list):
                name = ""
                for t in k["target"]:
                    if name != "":
                        name += "."
                    name += t
            self.key.append(name, k["match_type"])
        actions = jsonTable["actions"]
        action_ids = jsonTable["action_ids"]
        for i in range(0, len(actions)):
            actionName = actions[i]
            actionId = action_ids[i]
            self.actions[actionName] = actionId
    def __str__(self):
        return self.name
    def makeKeyInstance(self):
        return TableKeyInstance(self.key)

# Represents enough about the program executed to be
# able to invoke the BMV2 simulator, create a CLI file
# and test packets in pcap files.
class RunBMV2(object):
    def __init__(self, folder, options, jsonfile):

        self.clifile = folder + "/cli.txt"
        self.jsonfile = jsonfile
        self.stffile = None
        self.folder = folder
        self.pcapPrefix = "pcap"
        self.interfaces = {}
        self.expected = {}  # for each interface number of packets expected
        self.expectedAny = []  # interface on which any number of packets is fine
        self.packetDelay = 0
        self.options = options
        self.json = None
        self.tables = []
        self.actions = []
        self.switchLogFile = "switch.log"  # .txt is added by BMv2
        self.readJson()
        self.cmd_line_args = getattr(options, 'switchOptions', ())
        self.target_specific_cmd_line_args = getattr(options, 'switchTargetSpecificOptions', ())

    def readJson(self):
        with open(self.jsonfile) as jf:
            self.json = json.load(jf)
        for a in self.json["actions"]:
            self.actions.append(BMV2Action(a))
        for t in self.json["pipelines"][0]["tables"]:
            self.tables.append(BMV2Table(t))
        for t in self.json["pipelines"][1]["tables"]:
            self.tables.append(BMV2Table(t))
    def filename(self, interface, direction):
        return self.folder + "/" + self.pcapPrefix + str(interface) + "_" + direction + ".pcap"
    def interface_of_filename(self, f):
        return int(os.path.basename(f).rstrip('.pcap').lstrip(self.pcapPrefix).rsplit('_', 1)[0])
    def do_cli_command(self, cmd):
        if self.options.verbose:
            print(cmd)
        self.cli_stdin.write(cmd + "\n")
        self.cli_stdin.flush()
        self.packetDelay = 1
    def do_command(self, cmd):
        if self.options.verbose and cmd != "":
            print("STF Command:", cmd)
        first, cmd = nextWord(cmd)
        if first == "":
            pass
        elif first == "add":
            self.do_cli_command(self.parse_table_add(cmd))
        elif first == "setdefault":
            self.do_cli_command(self.parse_table_set_default(cmd))
        elif first == "mirroring_add":
            # Pass through mirroring_add commands unchanged, with same
            # arguments as expected by simple_switch_CLI
            self.do_cli_command(first + " " + cmd)
        elif first == "mc_mgrp_create" or first == "mc_node_create" or first == "mc_node_associate":
            # Pass through multicast group commands unchanged, with
            # same arguments as expected by simple_switch_CLI
            self.do_cli_command(first + " " + cmd)
        elif first == "packet":
            interface, data = nextWord(cmd)
            interface = int(interface)
            data = ''.join(data.split())
            time.sleep(self.packetDelay)
            try:
                self.interfaces[interface]._write_packet(HexToByte(data))
            except ValueError:
                reportError("Invalid packet data", data)
                return FAILURE
            self.interfaces[interface].flush()
            self.packetDelay = 0
        elif first == "expect":
            interface, data = nextWord(cmd)
            interface = int(interface)
            data = ''.join(data.split())
            if data != '':
                self.expected.setdefault(interface, []).append(data)
            else:
                self.expectedAny.append(interface)
        else:
            if self.options.verbose:
                print("ignoring stf command:", first, cmd)
    def parse_table_set_default(self, cmd):
        tableName, cmd = nextWord(cmd)
        table = self.tableByName(tableName)
        actionName, cmd = nextWord(cmd, "(")
        action = self.actionByName(table, actionName)
        actionArgs = action.makeArgsInstance()
        cmd = cmd.strip(")")
        while cmd != "":
            word, cmd = nextWord(cmd, ",")
            k, v = nextWord(word, ":")
            actionArgs.set(k, v)
        command = "table_set_default " + tableName + " " + actionName
        if actionArgs.size():
            command += " => " + str(actionArgs)
        return command
    def parse_table_add(self, cmd):
        tableName, cmd = nextWord(cmd)
        table = self.tableByName(tableName)
        key = table.makeKeyInstance()
        actionArgs = None
        actionName = None
        prio, cmd = nextWord(cmd)
        number = re.compile("[0-9]+")
        if not number.match(prio):
            # not a priority; push back
            cmd = prio + " " + cmd
            prio = ""
        while cmd != "":
            if actionName != None:
                # parsing action arguments
                word, cmd = nextWord(cmd, ",")
                k, v = nextWord(word, ":")
                actionArgs.set(k, v)
            else:
                # parsing table key
                word, cmd = nextWord(cmd)
                if cmd.find("=") >= 0:
                    # This command retrieves a handle for the key
                    # This feature is currently not supported, so we just ignore the handle part
                    cmd = cmd.split("=")[0]
                if word.find("(") >= 0:
                    # found action
                    actionName, arg = nextWord(word, "(")
                    action = self.actionByName(table, actionName)
                    actionArgs = action.makeArgsInstance()
                    cmd = arg + cmd
                    cmd = cmd.strip("()")
                else:
                    k, v = nextWord(word, ":")
                    key.set(k, v)
        if prio != "":
            # Priorities in BMV2 seem to be reversed with respect to the stf file
            # Hopefully 10000 is large enough
            prio = str(10000 - int(prio))
        command = "table_add " + table.name + " " + action.name + " " + str(key) + " => " + str(actionArgs)
        if table.match_type == "ternary":
            command += " " + prio
        return command
    def actionByName(self, table, actionName):
        for name, id in table.actions.items():
            action = self.actions[id]
            if action.name == actionName:
                return action
        # Try again with suffixes
        candidate = None
        for name, id in table.actions.items():
            action = self.actions[id]
            if action.name.endswith(actionName):
                if candidate is None:
                    candidate = action
                else:
                    raise Exception("Ambiguous action name " + actionName + " in " + table.name)
        if candidate is not None:
            return candidate

        raise Exception("No action", actionName, "in table", table)
    def tableByName(self, tableName):
        originalName = tableName
        for t in self.tables:
            if t.name == tableName:
                return t
        # If we can't find that try to match the tableName with a table suffix
        candidate = None
        for t in self.tables:
            if t.name.endswith(tableName):
                if candidate == None:
                    candidate = t
                else:
                    raise Exception("Table name " + tableName + " is ambiguous between " +
                                    candidate.name + " and " + t.name)
        if candidate is not None:
            return candidate
        raise Exception("Could not find table " + tableName)
    def interfaceArgs(self):
        # return list of interface names suitable for bmv2
        result = []
        for interface in sorted(self.interfaces):
            result.append("-i " + str(interface) + "@" + self.pcapPrefix + str(interface))
        return result
    def generate_model_inputs(self, stffile):
        self.stffile = stffile
        with open(stffile) as i:
            for line in i:
                line, comment = nextWord(line, "#")
                first, cmd = nextWord(line)
                if first == "packet" or first == "expect":
                    interface, cmd = nextWord(cmd)
                    interface = int(interface)
                    if not interface in self.interfaces:
                        # Can't open the interfaces yet, as that would block
                        ifname = self.interfaces[interface] = self.filename(interface, "in")
                        os.mkfifo(ifname)
        return SUCCESS
    def check_switch_server_ready(self, proc, thriftPort):
        """While the process is running, we check if the Thrift server has been
        started. If the Thrift server is ready, we assume that the switch was
        started successfully. This is only reliable if the Thrift server is
        started at the end of the init process"""
        while True:
            if proc.poll() is not None:
                return False
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(0.5)
            result = sock.connect_ex(("localhost", thriftPort))
            if result == 0:
                return  True
    def run(self):
        if self.options.verbose:
            print("Running model")
        wait = 0  # Time to wait before model starts running

        if self.options.usePsa:
            switch = "psa_switch"
            switch_cli = "psa_switch_CLI"
        else:
            switch = "simple_switch"
            switch_cli = "simple_switch_CLI"

        concurrent = ConcurrentInteger(os.getcwd(), 1000)
        rand = concurrent.generate()
        if rand is None:
            reportError("Could not find a free port for Thrift")
            return FAILURE
        thriftPort = str(9090 + rand)
        rv = SUCCESS
        try:
            os.remove("/tmp/bmv2-%d-notifications.ipc" % rand)
        except OSError:
            pass
        try:
            runswitch = [FindExe("behavioral-model", switch),
                         "--log-file", self.switchLogFile, "--log-flush",
                         "--use-files", str(wait), "--thrift-port", thriftPort,
                         "--device-id", str(rand)] + self.interfaceArgs() + ["../" + self.jsonfile]
            if self.cmd_line_args:
                runswitch += self.cmd_line_args
            if self.target_specific_cmd_line_args:
                runswitch += ['--',] + self.target_specific_cmd_line_args
            if self.options.verbose:
                print("Running", " ".join(runswitch))
            sw = subprocess.Popen(runswitch, cwd=self.folder)

            def openInterface(ifname):
                fp = self.interfaces[interface] = RawPcapWriter(ifname, linktype=0)
                fp._write_header(None)

            # Try to open input interfaces. Each time, we set a 2 second
            # timeout. If the timeout expires we check if the bmv2 process is
            # not running anymore. If it is, we check if we have exceeded the
            # one minute timeout (exceeding this timeout is very unlikely and
            # could mean the system is very slow for some reason). If one of the
            # 2 conditions above is met, the test is considered a FAILURE.
            start = time.time()
            sw_timeout = 60
            # open input interfaces
            # DANGER -- it is critical that we open these fifos in the same
            # order as bmv2, as otherwise we'll deadlock.  Would be nice if we
            # could open nonblocking.
            for interface in sorted(self.interfaces):
                ifname = self.interfaces[interface]
                while True:
                    try:
                        signal.alarm(2)
                        openInterface(ifname)
                        signal.alarm(0)
                    except TimeoutException:
                        if time.time() - start > sw_timeout:
                            return FAILURE
                        if sw.poll() is not None:
                            return FAILURE
                    else:
                        break

            # at this point we wait until the Thrift server is ready
            # also useful if there are no interfaces
            try:
                signal.alarm(int(sw_timeout + start - time.time()))
                self.check_switch_server_ready(sw, int(thriftPort))
                signal.alarm(0)
            except TimeoutException:
                return FAILURE
            time.sleep(0.1)

            runcli = [FindExe("behavioral-model", switch_cli), "--thrift-port", thriftPort]
            if self.options.verbose:
                print("Running", " ".join(runcli))

            try:
                cli = subprocess.Popen(runcli, cwd=self.folder, stdin=subprocess.PIPE)
                self.cli_stdin = cli.stdin
                with open(self.stffile) as i:
                    for line in i:
                        line, comment = nextWord(line, "#")
                        self.do_command(line)
                cli.stdin.close()
                for interface, fp in self.interfaces.iteritems():
                    fp.close()
                # Give time to the model to execute
                time.sleep(2)
                cli.terminate()
                sw.terminate()
                sw.wait()
            except Exception as e:
                cli.terminate()
                sw.terminate()
                sw.wait()
                raise e
            # This only works on Unix: negative returncode is
            # minus the signal number that killed the process.
            if sw.returncode != 0 and sw.returncode != -15:  # 15 is SIGTERM
                reportError(switch, "died with return code", sw.returncode);
                rv = FAILURE
            elif self.options.verbose:
                print(switch, "exit code", sw.returncode)
            cli.wait()
            if cli.returncode != 0 and cli.returncode != -15:
                reportError("CLI process failed with exit code", cli.returncode)
                rv = FAILURE
        finally:
            try:
                os.remove("/tmp/bmv2-%d-notifications.ipc" % rand)
            except OSError:
                pass
            concurrent.release(rand)
        if self.options.verbose:
            print("Execution completed")
        return rv
    def comparePacket(self, expected, received):
        received = convert_packet_bin2hexstr(received)
        expected = convert_packet_stf2hexstr(expected)
        strict_length_check = False
        if expected[-1] == '$':
            strict_length_check = True
            expected = expected[:-1]
        if len(received) < len(expected):
            reportError("Received packet too short", len(received), "vs",
                        len(expected), "(in units of hex digits)")
            reportError("Full expected packet is ", expected)
            reportError("Full received packet is ", received)
            return FAILURE
        for i in range(0, len(expected)):
            if expected[i] == "*":
                continue;
            if expected[i] != received[i]:
                reportError("Received packet ", received)
                reportError("Packet different at position", i, ": expected", expected[i], ", received", received[i])
                reportError("Full expected packet is ", expected)
                reportError("Full received packet is ", received)
                return FAILURE
        if strict_length_check and len(received) > len(expected):
            reportError("Received packet too long", len(received), "vs",
                        len(expected), "(in units of hex digits)")
            reportError("Full expected packet is ", expected)
            reportError("Full received packet is ", received)
            return FAILURE
        return SUCCESS
    def showLog(self):
        with open(self.folder + "/" + self.switchLogFile + ".txt") as a:
            log = a.read()
            print("Log file:")
            print(log)
    def checkOutputs(self):
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

            # Log packets.
            if self.options.observationLog:
                observationLog = open(self.options.observationLog, 'w')
                for pkt in packets:
                    observationLog.write('%d %s\n' % (
                        interface,
                        convert_packet_bin2hexstr(pkt)))
                observationLog.close()

            # Check for expected packets.
            if interface in self.expectedAny:
                if interface in self.expected:
                    reportError("Interface " + interface + " has both expected with packets and without")
                continue
            if interface not in self.expected:
                expected = []
            else:
                expected = self.expected[interface]
            if len(expected) != len(packets):
                reportError("Expected", len(expected), "packets on port", str(interface),
                            "got", len(packets))
                reportError("Full list of %d expected packets on port %d:"
                            "" % (len(expected), interface))
                for i in range(len(expected)):
                    reportError("    packet #%2d: %s"
                                "" % (i+1,
                                      convert_packet_stf2hexstr(expected[i])))
                reportError("Full list of %d received packets on port %d:"
                            "" % (len(packets), interface))
                for i in range(len(packets)):
                    reportError("    packet #%2d: %s"
                                "" % (i+1,
                                      convert_packet_bin2hexstr(packets[i])))
                self.showLog()
                return FAILURE
            for i in range(0, len(expected)):
                cmp = self.comparePacket(expected[i], packets[i])
                if cmp != SUCCESS:
                    reportError("Packet", i, "on port", str(interface), "differs")
                    return FAILURE
            # remove successfully checked interfaces
            if interface in self.expected:
                del self.expected[interface]
        if len(self.expected) != 0:
            # didn't find all the expects we were expecting
            reportError("Expected packects on ports", self.expected.keys(), "not received")
            return FAILURE
        else:
            return SUCCESS

def run_model(options, tmpdir, jsonfile, testfile):
    bmv2 = RunBMV2(tmpdir, options, jsonfile)
    result = bmv2.generate_model_inputs(testfile)
    if result != SUCCESS:
        return result
    result = bmv2.run()
    if result != SUCCESS:
        return result
    result = bmv2.checkOutputs()
    return result

######################### main

def usage(options):
    print("usage:", options.binary, "[-v] [-p] [-observation-log <file>] <json file> <stf file>");

def main(argv):
    options = Options()
    options.binary = argv[0]
    argv = argv[1:]
    while len(argv) > 0 and argv[0][0] == '-':
        if argv[0] == "-b":
            options.preserveTmp = True
        elif argv[0] == "-v":
            options.verbose = True
        elif argv[0] == "-p":
            options.usePsa = True
        elif argv[0] == '-observation-log':
            if len(argv) == 1:
                reportError("Missing argument", argv[0])
                usage(options)
                sys.exit(1)
            options.observationLog = argv[1]
            argv = argv[1:]
        else:
            reportError("Unknown option ", argv[0])
            usage(options)
        argv = argv[1:]
    if len(argv) < 2:
        usage(options)
        return FAILURE
    if not os.path.isfile(argv[0]) or not os.path.isfile(argv[1]):
        usage(options)
        return FAILURE

    tmpdir = tempfile.mkdtemp(dir=".")
    result = run_model(options, tmpdir, argv[0], argv[1])
    if options.preserveTmp:
        print("preserving", tmpdir)
    else:
        shutil.rmtree(tmpdir)
    if options.verbose:
        if result == SUCCESS:
            print("SUCCESS")
        else:
            print("FAILURE", result)
    return result

if __name__ == "__main__":
    sys.exit(main(sys.argv))

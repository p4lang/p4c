#!/usr/bin/env python3
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

import json
import os
import random
import re
import signal
import socket
import subprocess
import sys
import time
from collections import OrderedDict
from glob import glob
from pathlib import Path

try:
    import scapy.utils as scapy_util
    from scapy.layers.all import *
except ImportError:
    pass

FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../tools")))
import testutils
from stf.stf_parser import STFLexer, STFParser


class TimeoutException(Exception):
    pass


def signal_handler(signum, frame):
    raise TimeoutException("Timed out!")


signal.signal(signal.SIGALRM, signal_handler)


class Options(object):
    def __init__(self):
        self.binary = None
        self.verbose = False
        self.preserveTmp = False
        self.observationLog = None
        self.usePsa = False


def ByteToHex(byteStr):
    return "".join([("%02X" % x) for x in byteStr])


def convert_packet_bin2hexstr(pkt_bin):
    return ByteToHex(bytes(pkt_bin))


def convert_packet_stf2hexstr(pkt_stf_text):
    return "".join(pkt_stf_text.split()).upper()


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
                    n = os.path.join(dName, exe)
                    if os.path.isfile(n) and os.access(n, os.X_OK):
                        n_time = os.path.getmtime(n)
                        if n_time > rv_time:
                            rv = n
                            rv_time = n_time
            if rv is not None:
                return rv
        dir = os.path.dirname(dir)
    return exe


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
        for f, t in tableKey.fields.items():
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
        array = re.compile("(.*)\$([0-9]+)(.*)")
        m = array.match(key)
        if m:
            key = m.group(1) + "[" + m.group(2) + "]" + m.group(3)

        found = False
        if key in self.key.fields:
            found = True
        elif key + "$" in self.key.fields:
            key = key + "$"
            found = True
        elif key + ".$valid$" in self.key.fields:
            key = key + ".$valid$"
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
            testutils.log.info(self.key.fields)
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
            raise Exception("Decimal value " + value + " not supported for ternary key")
            return value
        values = "0123456789abcdefABCDEF*"
        replacements = (mask * 22) + "0"
        trans = str.maketrans(values, replacements)
        m = value.translate(trans)
        return prefix + value.replace("*", "0") + "&&&" + prefix + m

    def makeLpm(self, value):
        if value.find("/") >= 0:
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
        digits = len(value) - 2 - value.count("*")
        return value.replace("*", "0") + "/" + str(digits * bits_per_digit)

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
            name = ""
            if "name" in k:
                name = k["name"]
            else:
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


class BMv2StfLexer(STFLexer):
    def __init__(self):
        super().__init__()
        tokens = [
            "MIRRORING_ADD",
            "MIRRORING_ADD_MC",
            "MIRRORING_DELETE",
            "MIRRORING_GET",
            "MC_MGRP_CREATE",
            "MC_NODE_CREATE",
            "MC_NODE_UPDATE",
            "MC_NODE_ASSOCIATE",
            "COUNTER_READ",
            "COUNTER_WRITE",
            "REGISTER_READ",
            "REGISTER_WRITE",
            "REGISTER_RESET",
            "METER_GET_RATES",
            "METER_SET_RATES",
            "METER_ARRAY_SET_RATES",
        ]
        self.keywords.extend(tokens)
        for keyword in tokens:
            self.keywords_map[keyword.lower()] = keyword
        self.tokens.extend(tokens)


# PARSER GRAMMAR --------------------------------------------------------------


# statement : ADD qualified_name priority match_list action [= ID]
#           | ADD qualified_name match_list action [= ID]
#           | REMOVE [ALL | entry]
#           | CHECK_COUNTER ID(id_or_index) [count_type logical_cond number]
#           | EXPECT port expect_data
#           | EXPECT port
#           | NO_PACKET
#           | PACKET port packet_data
#           | SETDEFAULT qualified_name action
#           | WAIT
#           | direct_cmd
#
# direct_cmd : MIRRORING_ADD number number
#           | MIRRORING_ADD_MC number number
#           | MIRRORING_DELETE number
#           | MIRRORING_GET number
#           | MC_MGRP_CREATE number
#           | MC_NODE_CREATE number number_list
#           | MC_NODE_UPDATE number number_list
#           | MC_NODE_ASSOCIATE number number
#           | COUNTER_READ qualified_name number
#           | COUNTER_WRITE qualified_name number number number
#           | REGISTER_READ qualified_name number
#           | REGISTER_WRITE qualified_name number number
#           | REGISTER_RESET qualified_name
#           | METER_GET_RATES qualified_name number
#           | METER_SET_RATES qualified_name number meter_rate
#           | METER_ARRAY_SET_RATES qualified_name meter_rate
#
# meter_rate : number COLON number meter_rate
#           | number COLON number
class Bmv2StfParser(STFParser):
    def __init__(self):
        super().__init__(lexer=BMv2StfLexer())

    def p_statement_direct_cmd(self, p):
        "statement : direct_cmd"
        p[0] = p[1]

    def p_statement_check_counter(self, p):
        "statement : CHECK_COUNTER ID LPAREN id_or_index RPAREN"
        p[0] = (p[1].lower(), p[2], p[4], (None, "==", 0))

    def p_statement_check_counter_with_check(self, p):
        "statement : CHECK_COUNTER ID LPAREN id_or_index RPAREN count_type logical_cond number"
        p[0] = (p[1].lower(), p[2], p[4], (p[6], p[7], p[8]))

    def p_direct_cmd_mirroring_add(self, p):
        "direct_cmd : MIRRORING_ADD number number"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_mirroring_add_mc(self, p):
        "direct_cmd : MIRRORING_ADD_MC number number"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_mirroring_delete(self, p):
        "direct_cmd : MIRRORING_DELETE number"
        p[0] = (p[1].lower(), p[2])

    def p_direct_cmd_mirroring_get(self, p):
        "direct_cmd : MIRRORING_GET number"
        p[0] = (p[1].lower(), p[2])

    def p_direct_cmd_mc_mgrp_create(self, p):
        "direct_cmd : MC_MGRP_CREATE number"
        p[0] = (p[1].lower(), p[2])

    def p_direct_cmd_mc_node_create(self, p):
        "direct_cmd : MC_NODE_CREATE number number_list"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_mc_node_update(self, p):
        "direct_cmd : MC_NODE_UPDATE number number_list"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_mc_node_associate(self, p):
        "direct_cmd : MC_NODE_ASSOCIATE number number"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_counter_read(self, p):
        "direct_cmd : COUNTER_READ qualified_name number"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_counter_write(self, p):
        "direct_cmd : COUNTER_WRITE qualified_name number number number"
        p[0] = (p[1].lower(), p[2], p[3], p[4], p[5])

    def p_direct_cmd_register_read(self, p):
        "direct_cmd : REGISTER_READ qualified_name number"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_register_write(self, p):
        "direct_cmd : REGISTER_WRITE qualified_name number number"
        p[0] = (p[1].lower(), p[2], p[3], p[4])

    def p_direct_cmd_register_reset(self, p):
        "direct_cmd : REGISTER_RESET qualified_name"
        p[0] = (p[1].lower(), p[2])

    def p_direct_cmd_meter_get_rates(self, p):
        "direct_cmd : METER_GET_RATES qualified_name number"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_direct_cmd_meter_set_rates(self, p):
        "direct_cmd : METER_SET_RATES qualified_name number meter_rate"
        p[0] = (p[1].lower(), p[2], p[3], p[4])

    def p_direct_cmd_meter_array_set_rates(self, p):
        "direct_cmd : METER_ARRAY_SET_RATES qualified_name meter_rate"
        p[0] = (p[1].lower(), p[2], p[3])

    def p_meter_rate_many(self, p):
        "meter_rate : number_or_float COLON number meter_rate"
        p[0] = p[1] + ":" + p[3] + " " + p[4]

    def p_meter_rate_one(self, p):
        "meter_rate : number_or_float COLON number"
        p[0] = p[1] + ":" + p[3]


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
        self.packetDelay = 0
        self.options = options
        self.json = None
        self.tables = []
        self.actions = []
        self.switchLogFile = "switch.log"  # .txt is added by BMv2
        self.readJson()
        self.cmd_line_args = getattr(options, "switchOptions", ())
        self.target_specific_cmd_line_args = getattr(options, "switchTargetSpecificOptions", ())

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
        return int(os.path.basename(f).rstrip(".pcap").lstrip(self.pcapPrefix).rsplit("_", 1)[0])

    def do_cli_command(self, cmd):
        testutils.log.info("Sending '%s'", cmd)
        self.cli_stdin.write(bytes(cmd + "\n", encoding="utf8"))
        self.cli_stdin.flush()
        self.packetDelay = 1

    def execute_stf_command(self, stf_entry):
        if self.options.verbose and stf_entry:
            testutils.log.info("STF Command: %s", stf_entry)
        cmd = stf_entry[0]
        if cmd == "":
            pass
        elif cmd == "add":
            self.do_cli_command(self.parse_table_add(stf_entry))
        elif cmd == "setdefault":
            self.do_cli_command(self.parse_table_set_default(stf_entry))
        elif cmd in (
            "mirroring_add",
            "mirroring_add_mc",
            "mirroring_delete",
            "mirroring_get",
            "mc_mgrp_create",
            "mc_node_create",
            "mc_node_associate",
            "counter_read",
            "counter_write",
            "register_read",
            "register_write",
            "register_reset",
            "meter_get_rates",
            "meter_set_rates",
            "meter_array_set_rates",
        ):
            # Pass through multicast group commands unchanged, with
            # same arguments as expected by simple_switch_CLI
            self.do_cli_command(" ".join(stf_entry))
        elif cmd == "packet":
            interface = int(stf_entry[1])
            data = stf_entry[2]
            time.sleep(self.packetDelay)
            try:
                self.interfaces[interface]._write_packet(bytes.fromhex(data))
            except ValueError:
                testutils.log.error("Invalid packet data %s", data)
                return testutils.FAILURE
            self.interfaces[interface].flush()
            self.packetDelay = 0
        elif cmd == "expect":
            interface = int(stf_entry[1])
            pkt_data = stf_entry[2]
            self.expected.setdefault(interface, {})
            if pkt_data != "":
                self.expected[interface]["any"] = False
                self.expected[interface].setdefault("pkts", []).append(pkt_data)
            else:
                self.expected[interface]["any"] = True
        else:
            testutils.log.info("ignoring stf command: %s %s", cmd, stf_entry)

    def parse_table_set_default(self, cmd):
        # Gather objects
        tableName = cmd[1]
        actionCall = cmd[2]
        actionName = actionCall[0]
        actionArgs = actionCall[1]

        # Instantiate.
        tableInstance = self.tableByName(tableName)
        actionInstance = self.actionByName(tableInstance, actionName)
        actionArgsInstance = actionInstance.makeArgsInstance()
        for actionArg in actionArgs:
            actionArgsInstance.set(actionArg[0], actionArg[1])
        command = "table_set_default " + tableName + " " + actionName
        if actionArgsInstance.size():
            command += " " + str(actionArgsInstance)
        return command

    def parse_table_add(self, cmd):
        # Gather objects
        tableName = cmd[1]
        prio = cmd[2]
        keyList = cmd[3]
        actionCall = cmd[4]
        actionName = actionCall[0]
        actionArgs = actionCall[1]

        # Instantiate.
        tableInstance = self.tableByName(tableName)
        keyInstance = tableInstance.makeKeyInstance()
        for keyTuple in keyList:
            keyInstance.set(keyTuple[0], keyTuple[1])

        actionInstance = self.actionByName(tableInstance, actionName)
        actionArgsInstance = actionInstance.makeArgsInstance()
        for actionArg in actionArgs:
            actionArgsInstance.set(actionArg[0], actionArg[1])
        if prio:
            # Priorities in BMV2 seem to be reversed with respect to the stf file
            # Hopefully 10000 is large enough
            prio = str(10000 - int(prio))
        command = (
            "table_add "
            + tableInstance.name
            + " "
            + actionInstance.name
            + " "
            + str(keyInstance)
            + " => "
            + str(actionArgsInstance)
        )
        if tableInstance.match_type == "ternary":
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
                    raise Exception(
                        "Table name "
                        + tableName
                        + " is ambiguous between "
                        + candidate.name
                        + " and "
                        + t.name
                    )
        if candidate is not None:
            return candidate
        raise Exception("Could not find table " + tableName)

    def interfaceArgs(self):
        # return list of interface names suitable for bmv2
        result = []
        for interface in sorted(self.interfaces):
            result.append("-i " + str(interface) + "@" + self.pcapPrefix + str(interface))
        return result

    def generate_model_inputs(self, stf_map):
        for entry in stf_map:
            cmd = entry[0]
            if cmd in ["packet", "expect"]:
                interface = int(entry[1])
                if interface not in self.interfaces:
                    # Can't open the interfaces yet, as that would block
                    ifname = self.interfaces[interface] = self.filename(interface, "in")
                    os.mkfifo(ifname)
        return testutils.SUCCESS

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
                return True

    def run(self, stf_map):
        testutils.log.info("Running model")
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
            testutils.log.error("Could not find a free port for Thrift")
            return testutils.FAILURE
        thriftPort = str(9090 + rand)
        rv = testutils.SUCCESS
        try:
            os.remove("/tmp/bmv2-%d-notifications.ipc" % rand)
        except OSError:
            pass
        try:
            runswitch = (
                [
                    FindExe("behavioral-model", switch),
                    "--log-file",
                    self.switchLogFile,
                    "--log-flush",
                    "--use-files",
                    str(wait),
                    "--thrift-port",
                    thriftPort,
                    "--device-id",
                    str(rand),
                ]
                + self.interfaceArgs()
                + ["../" + self.jsonfile]
            )
            if self.cmd_line_args:
                runswitch += self.cmd_line_args
            if self.target_specific_cmd_line_args:
                runswitch += [
                    "--",
                ] + self.target_specific_cmd_line_args
                testutils.log.info("Running %s", " ".join(runswitch))
            sw = subprocess.Popen(runswitch, cwd=self.folder)

            def openInterface(ifname):
                fp = self.interfaces[interface] = scapy_util.RawPcapWriter(ifname, linktype=0)
                fp._write_header(None)

            # Try to open input interfaces. Each time, we set a 2 second
            # timeout. If the timeout expires we check if the bmv2 process is
            # not running anymore. If it is, we check if we have exceeded the
            # one minute timeout (exceeding this timeout is very unlikely and
            # could mean the system is very slow for some reason). If one of the
            # 2 conditions above is met, the test is considered a failure.
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
                            return testutils.FAILURE
                        if sw.poll() is not None:
                            return testutils.FAILURE
                    else:
                        break

            # at this point we wait until the Thrift server is ready
            # also useful if there are no interfaces
            try:
                signal.alarm(int(sw_timeout + start - time.time()))
                self.check_switch_server_ready(sw, int(thriftPort))
                signal.alarm(0)
            except TimeoutException:
                return testutils.FAILURE
            time.sleep(0.1)

            runcli = [
                FindExe("behavioral-model", switch_cli),
                "--thrift-port",
                thriftPort,
            ]
            testutils.log.info("Running %s", " ".join(runcli))

            try:
                cli = subprocess.Popen(runcli, cwd=self.folder, stdin=subprocess.PIPE)
                self.cli_stdin = cli.stdin
                for stf_cmd in stf_map:
                    self.execute_stf_command(stf_cmd)
                cli.stdin.close()
                for interface, fp in self.interfaces.items():
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
                testutils.log.error("%s died with return code %s", switch, sw.returncode)
                rv = testutils.FAILURE
            else:
                testutils.log.info("%s: exit code %s", switch, sw.returncode)
            cli.wait()
            if cli.returncode != 0 and cli.returncode != -15:
                testutils.log.error("CLI process failed with exit code %s", cli.returncode)
                rv = testutils.FAILURE
        finally:
            try:
                os.remove("/tmp/bmv2-%d-notifications.ipc" % rand)
            except OSError:
                pass
            concurrent.release(rand)
        testutils.log.info("Execution completed")
        return rv

    def showLog(self):
        with open(self.folder + "/" + self.switchLogFile + ".txt") as a:
            log = a.read()
            testutils.log.info("Log file:\n%s", log)

    def checkOutputs(self):
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
                # If the expected_pkt is None, the content does not matter.
                # We only care that the packet was received on this particular port.
                if not expected_pkt:
                    continue
                cmp_result = testutils.compare_pkt(expected_pkt, packets[idx])
                if cmp_result != testutils.SUCCESS:
                    testutils.log.error("Packet %s on port %s differs", idx, interface)
                    return cmp_result
            # Remove successfully checked interfaces
            if interface in self.expected:
                del self.expected[interface]
        if len(self.expected) != 0:
            # Didn't find all the expects we were expecting
            testutils.log.error("Expected packets on port(s) %s not received", self.expected.keys())
            return testutils.FAILURE
        testutils.log.info("All went well.")
        return testutils.SUCCESS

    def parse_stf_file(self, testfile):
        with open(testfile) as raw_stf:
            parser = Bmv2StfParser()
            stf_str = raw_stf.read()
            return parser.parse(stf_str)

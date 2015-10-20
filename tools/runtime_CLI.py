#!/usr/bin/python

# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Antonin Bas (antonin@barefootnetworks.com)
#
#

import argparse
import cmd
import os
import sys
import struct
import json

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol import TMultiplexedProtocol

from bm_runtime.standard import Standard
from bm_runtime.standard.ttypes import *
try:
    from bm_runtime.simple_pre import SimplePre
except:
    pass
try:
    from bm_runtime.simple_pre_lag import SimplePreLAG
except:
    pass

def enum(type_name, *sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    reverse = dict((value, key) for key, value in enums.iteritems())

    @staticmethod
    def to_str(x):
        return reverse[x]
    enums['to_str'] = to_str

    @staticmethod
    def from_str(x):
        return enums[x]

    enums['from_str'] = from_str
    return type(type_name, (), enums)

PreType = enum('PreType', 'None', 'SimplePre', 'SimplePreLAG')
MeterType = enum('MeterType', 'packets', 'bytes')

def bytes_to_string(byte_array):
    form = 'B' * len(byte_array)
    return struct.pack(form, *byte_array)

def table_error_name(x):
    return TableOperationErrorCode._VALUES_TO_NAMES[x]

def get_parser():

    class ActionToPreType(argparse.Action):
        def __init__(self, option_strings, dest, nargs=None, **kwargs):
            if nargs is not None:
                raise ValueError("nargs not allowed")
            super(ActionToPreType, self).__init__(option_strings, dest, **kwargs)

        def __call__(self, parser, namespace, values, option_string=None):
            assert(type(values) is str)
            setattr(namespace, self.dest, PreType.from_str(values))

    parser = argparse.ArgumentParser(description='BM runtime CLI')
    # One port == one device !!!! This is not a multidevice CLI
    parser.add_argument('--thrift-port', help='Thrift server port for table updates',
                        type=int, action="store", default=9090)

    parser.add_argument('--thrift-ip', help='Thrift IP address for table updates',
                        type=str, action="store", default='localhost')

    parser.add_argument('--json', help='JSON description of P4 program',
                        type=str, action="store", required=True)

    parser.add_argument('--pre', help='Packet Replication Engine used by target',
                        type=str, choices=['None', 'SimplePre', 'SimplePreLAG'],
                        default=PreType.SimplePre, action=ActionToPreType)
    
    return parser

TABLES = {}
ACTIONS = {}
METER_ARRAYS = {}

class MatchType:
    EXACT = 0
    LPM = 1
    TERNARY = 2
    VALID = 3 # not yet supported

    @staticmethod
    def to_str(x):
        return {0: "exact", 1: "lpm", 2: "ternary", 3: "valid"}[x]

    @staticmethod
    def from_str(x):
        return {"exact": 0, "lpm": 1, "ternary": 2, "valid": 3}[x]

class Table:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.match_type_ = None
        self.actions = {}
        self.key = []
        self.default_action = None

        TABLES[name] = self

    def num_key_fields(self):
        return len(self.key)

    def key_str(self):
        return ",\t".join([name + "(" + MatchType.to_str(t) + ", " + str(bw) + ")" for name, t, bw in self.key])
        
    def table_str(self):
        return "{0:30} [{1}]".format(self.name, self.key_str())

class Action:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.runtime_data = []

        ACTIONS[name] = self

    def num_params(self):
        return len(self.runtime_data)

    def runtime_data_str(self):
        return ",\t".join([name + "(" + str(bw) + ")" for name, bw in self.runtime_data])

    def action_str(self):
        return "{0:30} [{1}]".format(self.name, self.runtime_data_str())

class MeterArray:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.type_ = None
        self.size = None
        self.rate_count = None

        METER_ARRAYS[name] = self

    def meter_str(self):
        return "{0:30} [{1}, {2}]".format(self.name, self.size,
                                          MeterType.to_str(self.type_))

def load_json(json_src):
    def get_header_type(header_name, j_headers):
        for h in j_headers:
            if h["name"] == header_name:
                return h["header_type"]
        assert(0)

    def get_field_bitwidth(header_type, field_name, j_header_types):
        for h in j_header_types:
            if h["name"] != header_type: continue
            for f, bw in h["fields"]:
                if f == field_name:
                    return bw
        assert(0)

    with open(json_src, 'r') as f:
        json_ = json.load(f)

        for j_action in json_["actions"]:
            action = Action(j_action["name"], j_action["id"])
            for j_param in j_action["runtime_data"]:
                action.runtime_data += [(j_param["name"], j_param["bitwidth"])]

        for j_pipeline in json_["pipelines"]:
            for j_table in j_pipeline["tables"]:
                table = Table(j_table["name"], j_table["id"])
                table.match_type = MatchType.from_str(j_table["match_type"])
                type_ = j_table["type"]
                # if type_ != "simple":
                #     assert(0 and "only 'simple' table type supported for now")
                for action in j_table["actions"]:
                    table.actions[action] = ACTIONS[action]
                for j_key in j_table["key"]:
                    target = j_key["target"]
                    match_type = MatchType.from_str(j_key["match_type"])
                    if match_type == MatchType.VALID:
                        field_name = target + "_valid"
                        bitwidth = 1
                    else:
                        field_name = ".".join(target)
                        header_type = get_header_type(target[0],
                                                      json_["headers"])
                        bitwidth = get_field_bitwidth(header_type, target[1],
                                                      json_["header_types"])
                    table.key += [(field_name, match_type, bitwidth)]

        for j_meter in json_["meter_arrays"]:
            meter_array = MeterArray(j_meter["name"], j_meter["id"])
            meter_array.size = j_meter["size"]
            meter_array.type_ = MeterType.from_str(j_meter["type"])
            meter_array.rate_count = j_meter["rate_count"]

def ipv4Addr_to_bytes(addr):
    if not '.' in addr:
        raise Exception()
    return [int(b) for b in addr.split('.')]

def macAddr_to_bytes(addr):
    if not ':' in addr:
        raise Exception()
    return [int(b, 16) for b in addr.split(':')]

def ipv6Addr_to_bytes(addr):
    from ipaddr import IPv6Address
    ip = IPv6Address(addr)
    return [ord(b) for b in ip.packed]

def int_to_bytes(i, num):
    byte_array = []
    while i > 0:
        byte_array.append(i % 256)
        i = i / 256
        num -= 1
    while num > 0:
        byte_array.append(0)
        num -= 1
    byte_array.reverse()
    return byte_array

def parse_param(input_str, bitwidth):
    if bitwidth == 32:
        try:
            return ipv4Addr_to_bytes(input_str)
        except:
            pass
    elif bitwidth == 48:
        try:
            return macAddr_to_bytes(input_str)
        except:
            pass
    elif bitwidth == 128:
        try:
            return ipv6Addr_to_bytes(input_str)
        except:
            pass
    return int_to_bytes(int(input_str, 0), (bitwidth + 7) / 8)

def parse_runtime_data(action, params):
    bitwidths = [bw for( _, bw) in action.runtime_data]
    byte_array = []
    for input_str, bitwidth in zip(params, bitwidths):
        byte_array += [bytes_to_string(parse_param(input_str, bitwidth))]
    return byte_array

_match_types_mapping = {
    MatchType.EXACT : BmMatchParamType.EXACT,
    MatchType.LPM : BmMatchParamType.LPM,
    MatchType.TERNARY : BmMatchParamType.TERNARY,
    MatchType.VALID : BmMatchParamType.VALID,
}

def parse_match_key(table, key_fields):
    params = []
    match_types = [t for (_, t, _) in table.key]
    bitwidths = [bw for (_, _, bw) in table.key]
    for idx, field in enumerate(key_fields):
        param_type = _match_types_mapping[match_types[idx]]
        bw = bitwidths[idx]
        if param_type == BmMatchParamType.EXACT:
            key = bytes_to_string(parse_param(field, bw))
            param = BmMatchParam(type = param_type,
                                 exact = BmMatchParamExact(key))
        elif param_type == BmMatchParamType.LPM:
            prefix, length = field.split("/")
            key = bytes_to_string(parse_param(prefix, bw))
            param = BmMatchParam(type = param_type,
                                 lpm = BmMatchParamLPM(key, int(length)))
        elif param_type == BmMatchParamType.TERNARY:
            key, mask = field.split("&&&")
            key = bytes_to_string(parse_param(key, bw))
            mask = bytes_to_string(parse_param(mask, bw))
            param = BmMatchParam(type = param_type,
                                 ternary = BmMatchParamTernary(key, mask))
        elif param_type == BmMatchParamType.VALID:
            key = bool(int(field))
            param = BmMatchParam(type = param_type,
                                 valid = BmMatchParamValid(key))
        else:
            assert(0)
        params.append(param)
    return params

def printable_byte_str(s):
    return ":".join("{:02x}".format(ord(c)) for c in s)

def BmMatchParam_to_str(self):
    return BmMatchParamType._VALUES_TO_NAMES[self.type] + "-" +\
        (self.exact.to_str() if self.exact else "") +\
        (self.lpm.to_str() if self.lpm else "") +\
        (self.ternary.to_str() if self.ternary else "") +\
        (self.valid.to_str() if self.valid else "")

def BmMatchParamExact_to_str(self):
    return printable_byte_str(self.key)

def BmMatchParamLPM_to_str(self):
    return printable_byte_str(self.key) + "/" + str(self.prefix_length)

def BmMatchParamTernary_to_str(self):
    return printable_byte_str(self.key) + " &&& " + printable_byte_str(self.mask)

def BmMatchParamValid_to_str(self):
    return ""

BmMatchParam.to_str = BmMatchParam_to_str
BmMatchParamExact.to_str = BmMatchParamExact_to_str
BmMatchParamLPM.to_str = BmMatchParamLPM_to_str
BmMatchParamTernary.to_str = BmMatchParamTernary_to_str
BmMatchParamValid.to_str = BmMatchParamValid_to_str

# services is [(service_name, client_class), ...]
def thrift_connect(thrift_ip, thrift_port, services):
    # Make socket
    transport = TSocket.TSocket(thrift_ip, thrift_port)
    # Buffering is critical. Raw sockets are very slow
    transport = TTransport.TBufferedTransport(transport)
    # Wrap in a protocol
    bprotocol = TBinaryProtocol.TBinaryProtocol(transport)

    clients = []

    for service_name, service_cls in services:
        if service_name is None:
            clients.append(None)
            continue
        protocol = TMultiplexedProtocol.TMultiplexedProtocol(bprotocol, service_name)
        client = service_cls(protocol)
        clients.append(client)

    # Connect!
    try:
        transport.open()
    except TTransport.TTransportException:
        print "Could not connect to thrift client on port", thrift_port
        print "Make sure the switch is running and that you have the right port"
        sys.exit(1)

    return clients

class RuntimeAPI(cmd.Cmd):
    prompt = 'RuntimeCmd: '
    intro = "Control utility for runtime P4 table manipulation"
    
    @staticmethod
    def get_thrift_services(pre_type):
        services = [("standard", Standard.Client)]

        if pre_type == PreType.SimplePre:
            services += [("simple_pre", SimplePre.Client)]
        elif pre_type == PreType.SimplePreLAG:
            services += [("simple_pre_lag", SimplePreLAG.Client)]
        else:
            services += [(None, None)]

        return services

    def __init__(self, pre_type, standard_client, mc_client=None):
        cmd.Cmd.__init__(self)
        self.client = standard_client
        self.mc_client = mc_client
        self.pre_type = pre_type

    def do_greet(self, line):
        print "hello"
    
    def do_EOF(self, line):
        print
        return True

    def do_shell(self, line):
        "Run a shell command"
        output = os.popen(line).read()
        print output

    def do_show_tables(self, line):
        "List tables defined in the P4 program"
        for table_name in sorted(TABLES):
            print TABLES[table_name].table_str()

    def do_show_actions(self, line):
        "List tables defined in the P4 program"
        for action_name in sorted(ACTIONS):
            print ACTIONS[action_name].action_str()

    def _complete_tables(self, text):
        tables = sorted(TABLES.keys())
        if not text:
            return tables
        return [t for t in tables if t.startswith(text)]

    def do_table_show_actions(self, line):
        "List tables defined in the P4 program"
        table = TABLES[line]
        for action_name in sorted(table.actions):
            print ACTIONS[action_name].action_str()

    def complete_table_show_actions(self, text, line, start_index, end_index):
        return self._complete_tables(self, text)

    def do_table_info(self, line):
        "Show info about a table"
        table = TABLES[line]
        print table.table_str()
        print "*" * 80
        for action_name in sorted(table.actions):
            print ACTIONS[action_name].action_str()

    def complete_table_info(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    def _complete_actions(self, text, table_name = None):
        if not table_name:
            actions = sorted(ACTIONS.keys())
        elif table_name not in TABLES:
            return []
        actions = sorted(TABLES[table_name].actions.keys())
        if not text:
            return actions
        return [a for a in actions if a.startswith(text)]

    def _complete_table_and_action(self, text, line):
        tables = sorted(TABLES.keys())
        args = line.split()
        args_cnt = len(args)
        if args_cnt == 1 and not text:
            return self._complete_tables(text)
        if args_cnt == 2 and text:
            return self._complete_tables(text)
        table_name = args[1]
        if args_cnt == 2 and not text:
            return self._complete_actions(text, table_name)
        if args_cnt == 3 and text:
            return self._complete_actions(text, table_name)
        return []

    def do_table_set_default(self, line):
        "Set default action for a match table: table_set_default <table name> <action name> <action parameters>"
        args = line.split()
        table_name, action_name = args[0], args[1]
        if table_name not in TABLES:
            print "Invalid table"
            return
        table = TABLES[table_name]
        if action_name not in table.actions:
            print "Invalid action"
            return
        action = ACTIONS[action_name]
        if len(args[2:]) != action.num_params():
            print "Action", action_name, "needs", action.num_params(), "parameters"
            return
        try:
            runtime_data = parse_runtime_data(action, args[2:])
        except:
            print "Invalid parameter"
            return
        print "Setting default action of", table_name
        print "{0:20} {1}".format("action:", action_name)
        print "{0:20} {1}".format(
            "runtime data:",
            "\t".join(printable_byte_str(d) for d in runtime_data)
        )
        self.client.bm_mt_set_default_action(table_name, action_name, runtime_data)
        print "SUCCESS"

    def complete_table_set_default(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    def do_table_add(self, line):
        "Add entry to a match table: table_add <table name> <action name> <match fields> => <action parameters> [priority]"
        args = line.split()
        table_name, action_name = args[0], args[1]
        if table_name not in TABLES:
            print "Invalid table"
            return
        table = TABLES[table_name]
        if action_name not in table.actions:
            print "Invalid action"
            return
        if table.match_type == MatchType.TERNARY:
            priority = int(args.pop(-1))
        else:
            priority = 0
        action = ACTIONS[action_name]
        for idx, input_ in enumerate(args[2:]):
            if input_ == "=>": break
        idx += 2
        match_key = args[2:idx]
        action_params = args[idx+1:]
        if len(match_key) != table.num_key_fields():
            print "Table", table_name, "needs", table.num_key_fields(), "key fields"
            return
        if len(action_params) != action.num_params():
            print "Action", action_name, "needs", action.num_params(), "parameters"
            return

        try:
            runtime_data = parse_runtime_data(action, action_params)
        except:
            print "Invalid parameter"
            return

        print "Adding entry to", MatchType.to_str(table.match_type), "match table", table_name

        match_key = parse_match_key(table, match_key)

        print "{0:20} {1}".format(
            "match key:",
            "\t".join(d.to_str() for d in match_key)
        )
        print "{0:20} {1}".format("action:", action_name)
        print "{0:20} {1}".format(
            "runtime data:",
            "\t".join(printable_byte_str(d) for d in runtime_data)
        )

        entry_handle = self.client.bm_mt_add_entry(
            table_name, match_key, action_name, runtime_data,
            BmAddEntryOptions(priority = priority)
        )
        print "SUCCESS"
        print "entry has been added with handle", entry_handle

    def complete_table_add(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    def do_table_delete(self, line):
        "Delete entry from a match table: table_delete <table name> <entry handle>"
        args = line.split()
        table_name = args[0]
        if table_name not in TABLES:
            print "Invalid table"
            return
        entry_handle = int(args[1])
        print "Deleting entry", entry_handle, "from", table_name
        self.client.bm_mt_delete_entry(table_name, entry_handle)
        print "SUCCESS"

    def complete_table_delete(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    def do_mc_mgrp_create(self, line):
        "Create multicast group: mc_mgrp_create <group id>"
        assert(self.pre_type != PreType.None)
        mgrp = int(line.split()[0])
        print "Creating multicast group", mgrp
        mgrp_hdl = self.mc_client.bm_mc_mgrp_create(mgrp)
        print "SUCCESS"
        assert(mgrp == mgrp_hdl)

    def do_mc_mgrp_destroy(self, line):
        "Destroy multicast group: mc_mgrp_destroy <group id>"
        assert(self.pre_type != PreType.None)
        mgrp = int(line.split()[0])
        print "Destroying multicast group", mgrp
        self.mc_client.bm_mc_mgrp_destroy(mgrp)
        print "SUCCESS"

    def ports_to_port_map_str(self, ports):
        last_port_num = 0
        port_map_str = ""
        for port_num_str in ports:
            port_num = int(port_num_str)
            port_map_str += "0" * (port_num - last_port_num) + "1"
            last_port_num = port_num
        return port_map_str[::-1]

    def parse_ports_and_lags(self, args):
        ports = []
        i = 1
        while (i < len(args) and args[i] != '|'):
            ports.append(args[i])
            i += 1
        port_map_str = self.ports_to_port_map_str(ports)
        if self.pre_type == PreType.SimplePreLAG:
            i += 1
            lags = [] if i == len(args) else args[i:]
            lag_map_str = self.ports_to_port_map_str(lags)
        else:
            lag_map_str = None
        return port_map_str, lag_map_str

    def do_mc_node_create(self, line):
        "Create multicast node: mc_node_create <rid> <space-separated port list> [ | <space-separated lag list> ]"
        assert(self.pre_type != PreType.None)
        args = line.split()
        rid = int(args[0])
        port_map_str, lag_map_str = self.parse_ports_and_lags(args)
        if self.pre_type == PreType.SimplePre:
            print "Creating node with rid", rid, "and with port map", port_map_str
            l1_hdl = self.mc_client.bm_mc_node_create(rid, port_map_str)
        else:
            print "Creating node with rid", rid, ", port map", port_map_str, "and lag map", lag_map_str
            l1_hdl = self.mc_client.bm_mc_node_create(rid, port_map_str, lag_map_str)
        print "SUCCESS"
        print "node was created with handle", l1_hdl

    def do_mc_node_update(self, line):
        "Update multicast node: mc_node_update <node handle> <space-separated port list> [ | <space-separated lag list> ]"
        assert(self.pre_type != PreType.None)
        args = line.split()
        l1_hdl = int(args[0])
        port_map_str, lag_map_str = self.parse_ports_and_lags(args)
        if self.pre_type == PreType.SimplePre:
            print "Updating node", l1_hdl, "with port map", port_map_str
            self.mc_client.bm_mc_node_update(l1_hdl, port_map_str)
        else:
            print "Updating node", l1_hdl, "with port map", port_map_str, "and lag map", lag_map_str
            self.mc_client.bm_mc_node_update(l1_hdl, port_map_str, lag_map_str)
        print "SUCCESS"

    def do_mc_node_associate(self, line):
        "Associate node to multicast group: mc_node_associate <node handle> <group handle>"
        assert(self.pre_type != PreType.None)
        args = line.split()
        mgrp = int(args[0])
        l1_hdl = int(args[1])
        print "Associating node", l1_hdl, "to multicast group", mgrp
        self.mc_client.bm_mc_node_associate(mgrp, l1_hdl)
        print "SUCCESS"

    def do_mc_node_dissociate(self, line):
        "Dissociate node from multicast group: mc_node_associate <node handle> <group handle>"
        assert(self.pre_type != PreType.None)
        args = line.split()
        mgrp = int(args[0])
        l1_hdl = int(args[1])
        print "Dissociating node", l1_hdl, "from multicast group", mgrp
        self.mc_client.bm_mc_node_dissociate(mgrp, l1_hdl)
        print "SUCCESS"

    def do_mc_node_destroy(self, line):
        "Destroy multicast node: mc_node_destroy <node handle>"
        assert(self.pre_type != PreType.None)
        l1_hdl = int(line.split()[0])
        print "Destroying node", l1_hdl
        self.mc_client.bm_mc_node_destroy(l1_hdl)
        print "SUCCESS"

    def do_mc_set_lag_membership(self, line):
        "Set lag membership of port list: mc_set_lag_membership <lag index> <space-separated port list>"
        assert(self.pre_type == PreType.SimplePreLAG)
        args = line.split()
        lag_index = int(args[0])
        port_map_str = self.ports_to_port_map_str(args[1:])
        print "Setting lag membership:", lag_index, "<-", port_map_str
        self.mc_client.bm_mc_set_lag_membership(lag_index, port_map_str)
        print "SUCCESS"

    def do_load_new_config_file(self, line):
        "Load new json config: load_new_config_file <path to .json file>"
        filename = line
        if not os.path.isfile(filename):
            print filename, "is not a valid file"
            return
        print "Loading new Json config"
        with open(filename, 'r') as f:
            json_str = f.read()
            try:
                json.loads(json_str)
            except:
                print filename, "is not a valid json file"
                return
            # TODO: catch exception
            self.client.bm_load_new_config(json_str)
        print "SUCCESS"

    def do_swap_configs(self, line):
        "Swap the 2 existing configs, need to have called load_new_config_file before"
        print "Swapping configs"
        # TODO: catch exception
        self.client.bm_swap_configs()
        print "SUCCESS"

    def do_meter_set_rates(self, line):
        "Configure rates for meter: meter_set_rates <name> <rate_1>:<burst_1> <rate_2>:<burst_2> ..."
        args = line.split()
        if len(args) < 1:
            print "Invalid number of args"
            return
        meter_name = args[0]
        if meter_name not in METER_ARRAYS:
            print "Invalid meter name"
            return
        rates = args[1:]
        meter = METER_ARRAYS[meter_name]
        if len(rates) != meter.rate_count:
            print "Invalid number of rates, expected", meter.rate_count, "but got", len(rates)
            return
        new_rates = []
        for rate in rates:
            try:
                r, b = rate.split(':')
                r = float(r)
                b = int(b)
                new_rates.append(BmMeterRateConfig(r, b))
            except:
                print "Error while parsing rates"
                return
        self.client.bm_meter_array_set_rates(meter_name, new_rates)
        print "SUCCESS"

    def complete_meter_set_rates(self, text, line, start_index, end_index):
        return self._complete_meters(text)

    def _complete_meters(self, text):
        meters = sorted(METER_ARRAYS.keys())
        if not text:
            return meters
        return [t for t in meters if t.startswith(text)]

    def do_dump_table(self, line):
        "Display some (non-formatted) information about a table"
        args = line.split()
        table_name = args[0]
        if table_name not in TABLES:
            print "Invalid table"
            return
        print self.client.bm_dump_table(table_name)
        print "SUCCESS"

    def complete_dump_table(self, text, line, start_index, end_index):
        return self._complete_tables(text)

def main():
    args = get_parser().parse_args()

    load_json(args.json)

    standard_client, mc_client = thrift_connect(
        args.thrift_ip, args.thrift_port,
        RuntimeAPI.get_thrift_services(args.pre)
    )

    RuntimeAPI(args.pre, standard_client, mc_client).cmdloop()

if __name__ == '__main__':
    main()

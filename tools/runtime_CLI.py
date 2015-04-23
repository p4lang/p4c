#!/usr/bin/python

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

from runtime import Runtime
from runtime.ttypes import *


def bytes_to_string(byte_array):
    form = 'B' * len(byte_array)
    return struct.pack(form, *byte_array)

def table_error_name(x):
    return TableOperationErrorCode._VALUES_TO_NAMES[x]

parser = argparse.ArgumentParser(description='BM runtime CLI')
# One port == one device !!!! This is not a multidevice CLI
parser.add_argument('--thrift-port', help='Thrift server port for table updates',
                    type=int, action="store", default=9090)

parser.add_argument('--json', help='JSON description of P4 program',
                    type=str, action="store", required=True)

args = parser.parse_args()

THRIFT_PORT = args.thrift_port

TABLES = {}
ACTIONS = {}

class MatchType:
    EXACT = 0
    LPM = 1
    TERNARY = 2

    @staticmethod
    def to_str(x):
        return {0: "exact", 1: "lpm", 2: "ternary"}[x]

    @staticmethod
    def from_str(x):
        return {"exact": 0, "lpm": 1, "ternary": 2}[x]

class Table:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.type_ = None
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
                table.type_ = MatchType.from_str(j_table["type"])
                for action in j_table["actions"]:
                    table.actions[action] = ACTIONS[action]
                for j_key in j_table["key"]:
                    target = j_key["target"]
                    field_name = ".".join(target)
                    header_type = get_header_type(target[0], json_["headers"])
                    bitwidth = get_field_bitwidth(header_type, target[1], json_["header_types"])
                    table.key += [(field_name,
                                   MatchType.from_str(j_key["match_type"]),
                                   bitwidth)]

def ipv4Addr_to_bytes(addr):
    return [int(b) for b in addr.split('.')]

def macAddr_to_bytes(addr):
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

def parse_match_key_exact(table, key_fields):
    match_types = [t for (_, t, _) in table.key]
    bitwidths = [bw for (_, _, bw) in table.key]
    byte_array = []
    for idx, field in enumerate(key_fields):
        match_type = match_types[idx]
        assert(match_type == MatchType.EXACT)
        bw = bitwidths[idx]
        byte_array += [bytes_to_string(parse_param(field, bw))]
    return byte_array

def parse_match_key_lpm(table, key_fields): 
    match_types = [t for (_, t, _) in table.key]
    bitwidths = [bw for (_, _, bw) in table.key]
    lpm_byte_array = None
    byte_array = []
    prefix_length = 0
    for idx, field in enumerate(key_fields):
        match_type = match_types[idx]
        assert(match_type == MatchType.EXACT or match_type == MatchType.LPM)
        bw = bitwidths[idx]
        if match_type == MatchType.EXACT:
            byte_array += [bytes_to_string(parse_param(field, bw))]
            prefix_length += bw
        elif match_type == MatchType.LPM:
            assert(not lpm_byte_array)
            prefix, length = field.split("/")
            lpm_byte_array = parse_param(prefix, bw)
            prefix_length += int(length)
    return [bytes_to_string(lpm_byte_array)] + byte_array, prefix_length

def gen_prefix_bytes(length, num_bytes):
    byte_array = []
    while length >= 8:
        byte_array.append(0xFF)
        length -= 8
        num_bytes -= 1
    if length > 0:
        byte_array.append((0xFF << (8 - length) & 0xFF))
        num_bytes -= 1
    assert(num_bytes >= 0)
    while num_bytes > 0:
        byte_array.append(0)
        num_bytes -= 1
    return byte_array

def parse_match_key_ternary(table, key_fields):
    match_types = [t for (_, t, _) in table.key]
    bitwidths = [bw for (_, _, bw) in table.key]
    byte_array = []
    mask_array = []
    for idx, field in enumerate(key_fields):
        match_type = match_types[idx]
        if match_type == MatchType.EXACT:
            byte_array += [bytes_to_string(parse_param(field, bw))]
            mask_array += [[0xFF] * ((bw + 7) / 8)]
        elif match_type == MatchType.LPM:
            prefix, length = field.split("/")
            byte_array += [bytes_to_string(parse_param(prefix, bw))]
            mask_array += [bytes_to_string(gen_prefix_bytes(length, (bw + 7) / 8))]
        elif match_type == MatchType.TERNARY:
            bytes_, mask = field.split("&&&")
            byte_array += [bytes_to_string(parse_param(bytes_, bw))]
            mask_array += [bytes_to_string(parse_param(mask, bw))]
    return byte_array, mask_array

def printable_byte_str(s):
    return ":".join("{:02x}".format(ord(c)) for c in s)
    
class RuntimeAPI(cmd.Cmd):
    prompt = 'RuntimeCmd: '
    intro = "Control utility for runtime P4 table manipulation"

    def __init__(self, thrift_client):
        cmd.Cmd.__init__(self)
        self.client = thrift_client

    def do_greet(self, line):
        print "hello"
    
    def do_EOF(self, line):
        print
        return True

    def do_shell(self, line):
        "Run a shell command"
        output = os.popen(line).read()
        print output

    def do_show_thrift_port(self, line):
        "Show thrift port used to send commands to Simple Router"
        print THRIFT_PORT

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
        self.client.bm_set_default_action(table_name, action_name, runtime_data)
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
        if table.type_ == MatchType.TERNARY:
            priority = int(args.pop(-1))
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

        print "Adding entry to", MatchType.to_str(table.type_), "match table", table_name

        if table.type_ == MatchType.EXACT:
            match_key = parse_match_key_exact(table, match_key)
        elif table.type_ == MatchType.LPM:
            match_key, prefix_length = parse_match_key_lpm(table, match_key)
        elif table.type_ == MatchType.TERNARY:
            match_key, mask = parse_match_key_ternary_match(table, match_key)

        print "{0:20} {1}".format(
            "match key:",
            "\t".join(printable_byte_str(d) for d in match_key)
        )
        print "{0:20} {1}".format("action:", action_name)
        print "{0:20} {1}".format(
            "runtime data:",
            "\t".join(printable_byte_str(d) for d in runtime_data)
        )

        if table.type_ == MatchType.EXACT:
            entry_handle = self.client.bm_table_add_exact_match_entry(
                table_name, action_name, match_key, runtime_data
            )
        elif table.type_ == MatchType.LPM:
            entry_handle = self.client.bm_table_add_lpm_entry(
                table_name, action_name, match_key, prefix_length, runtime_data
            )
        elif table.type_ == MatchType.TERNARY:
            entry_handle = self.client.bm_table_add_lpm_entry(
                table_name, action_name, match_key, mask, priority, runtime_data
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
        self.client.bm_table_delete_entry(table_name, entry_handle)
        print "SUCCESS"

    def complete_table_delete(self, text, line, start_index, end_index):
        return self._complete_tables(text)


    def do_mc_mgrp_create(self, line):
        mgrp = int(line.split()[0])
        print "Creating multicast group", mgrp
        mgrp_hdl = self.client.bm_mc_mgrp_create(mgrp)
        print "SUCCESS"
        assert(mgrp == mgrp_hdl)

    def do_mc_mgrp_destroy(self, line):
        mgrp = int(line.split()[0])
        print "Destroying multicast group", mgrp
        self.client.bm_mc_mgrp_destroy(mgrp)
        print "SUCCESS"

    def do_mc_l1_node_create(self, line):
        rid = int(line.split()[0])
        print "Creating l1 node with rid", rid
        l1_hdl = self.client.bm_mc_l1_node_create(rid)
        print "SUCCESS"
        print "l1 node was created with handle", l1_hdl

    def do_mc_l1_node_associate(self, line):
        mgrp = int(line.split()[0])
        l1_hdl = int(line.split()[1])
        print "Associating l1 node", l1_hdl, "to multicast group", mgrp
        self.client.bm_mc_l1_node_associate(mgrp, l1_hdl)
        print "SUCCESS"

    def do_mc_l1_node_destroy(self, line):
        l1_hdl = int(line.split()[0])
        print "Destroying l1 node", l1_hdl,
        self.client.bm_mc_l1_node_destroy(l1_hdl)
        print "SUCCESS"

    def ports_to_port_map_str(self, ports):
        last_port_num = 0
        port_map_str = ""
        for port_num_str in ports:
            port_num = int(port_num_str)
            port_map_str += "0" * (port_num - last_port_num) + "1"
            last_port_num = port_num
        return port_map_str[::-1]

    def do_mc_l2_node_create(self, line):
        args = line.split()
        l1_hdl = int(args[0])
        port_map_str = self.ports_to_port_map_str(args[1:])
        print "Creating l2 node for l1 node", l1_hdl, "with port map", port_map_str
        l2_hdl = self.client.bm_mc_l2_node_create(l1_hdl, port_map_str)
        print "SUCCESS"
        print "l2 node was created with handle", l2_hdl

    def do_mc_l2_node_update(self, line):
        args = line.split()
        l2_hdl = int(args[0])
        port_map_str = self.ports_to_port_map_str(args[1:])
        print "Updating l2 node", l2_hdl, "with port map", port_map_str
        self.client.bm_mc_l2_node_update(l2_hdl, port_map_str)
        print "SUCCESS"

    def do_mc_l2_node_destroy(self, line):
        l2_hdl = int(line.split()[0])
        print "Destroying l2 node", l2_hdl,
        self.client.bm_mc_l2_node_destroy(l2_hdl)
        print "SUCCESS"


def thrift_connect():
    # Make socket
    transport = TSocket.TSocket('localhost', THRIFT_PORT)
    # Buffering is critical. Raw sockets are very slow
    transport = TTransport.TBufferedTransport(transport)
    # Wrap in a protocol
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    # Create a client to use the protocol encoder
    client = Runtime.Client(protocol)
    # Connect!
    transport.open()

    return client


def main():
    load_json(args.json)

    try:
        client = thrift_connect()
    except TTransport.TTransportException:
        print "Could not connect to thrift client on port", THRIFT_PORT
        print "Make sure the switch is running and that you have the right port"
        sys.exit(1)

    RuntimeAPI(client).cmdloop()

if __name__ == '__main__':
    main()



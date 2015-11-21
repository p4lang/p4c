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
from functools import wraps

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
TableType = enum('TableType', 'simple', 'indirect', 'indirect_ws')

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
COUNTER_ARRAYS = {}
REGISTER_ARRAYS = {}

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
        self.type_ = None

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

class CounterArray:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.is_direct = None
        self.size = None
        self.binding = None

        COUNTER_ARRAYS[name] = self

    def counter_str(self):
        return "{0:30} [{1}]".format(self.name, self.size)

class RegisterArray:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.width = None
        self.size = None

        REGISTER_ARRAYS[name] = self

    def register_str(self):
        return "{0:30} [{1}]".format(self.name, self.size)

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
                table.type_ = TableType.from_str(j_table["type"])
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

        if "meter_arrays" in json_:
            for j_meter in json_["meter_arrays"]:
                meter_array = MeterArray(j_meter["name"], j_meter["id"])
                meter_array.size = j_meter["size"]
                meter_array.type_ = MeterType.from_str(j_meter["type"])
                meter_array.rate_count = j_meter["rate_count"]

        if "counter_arrays" in json_:
            for j_counter in json_["counter_arrays"]:
                counter_array = CounterArray(j_counter["name"], j_counter["id"])
                counter_array.is_direct = j_counter["is_direct"]
                if counter_array.is_direct:
                    counter_array.binding = j_counter["binding"]
                else:
                    counter_array.size = j_counter["size"]

        if "register_arrays" in json_:
            for j_register in json_["register_arrays"]:
                register_array = RegisterArray(j_register["name"],
                                               j_register["id"])
                register_array.size = j_register["size"]
                register_array.width = j_register["bitwidth"]


class UIn_Error(Exception):
    def __init__(self, info=""):
        self.info = info

    def __str__(self):
        return self.info

class UIn_ResourceError(UIn_Error):
    def __init__(self, res_type, name):
        self.res_type = res_type
        self.name = name

    def __str__(self):
        return "Invalid %s name (%s)" % (self.res_type, self.name)

class UIn_MatchKeyError(UIn_Error):
    def __init__(self, info=""):
        self.info = info

    def __str__(self):
        return self.info

class UIn_RuntimeDataError(UIn_Error):
    def __init__(self, info=""):
        self.info = info

    def __str__(self):
        return self.info

class CLI_FormatExploreError(Exception):
    def __init__(self):
        pass

class UIn_BadParamError(UIn_Error):
    def __init__(self, info=""):
        self.info = info

    def __str__(self):
        return self.info

class UIn_BadIPv4Error(UIn_Error):
    def __init__(self):
        pass

class UIn_BadIPv6Error(UIn_Error):
    def __init__(self):
        pass

class UIn_BadMacError(UIn_Error):
    def __init__(self):
        pass

def ipv4Addr_to_bytes(addr):
    if not '.' in addr:
        raise CLI_FormatExploreError()
    s = addr.split('.')
    if len(s) != 4:
        raise UIn_BadIPv4Error()
    try:
        return [int(b) for b in s]
    except:
        raise UIn_BadIPv4Error()

def macAddr_to_bytes(addr):
    if not ':' in addr:
        raise CLI_FormatExploreError()
    s = addr.split(':')
    if len(s) != 6:
        raise UIn_BadMacError()
    try:
        return [int(b, 16) for b in s]
    except:
        raise UIn_BadMacError()

def ipv6Addr_to_bytes(addr):
    from ipaddr import IPv6Address
    if not ':' in addr:
        raise CLI_FormatExploreError()
    try:
        ip = IPv6Address(addr)
    except:
        raise UIn_BadIPv6Error()
    try:
        return [ord(b) for b in ip.packed]
    except:
        raise UIn_BadIPv6Error()

def int_to_bytes(i, num):
    byte_array = []
    while i > 0:
        byte_array.append(i % 256)
        i = i / 256
        num -= 1
    if num < 0:
        raise UIn_BadParamError("Parameter is too large")
    while num > 0:
        byte_array.append(0)
        num -= 1
    byte_array.reverse()
    return byte_array

def parse_param(input_str, bitwidth):
    if bitwidth == 32:
        try:
            return ipv4Addr_to_bytes(input_str)
        except CLI_FormatExploreError:
            pass
        except UIn_BadIPv4Error:
            raise UIn_BadParamError("Invalid IPv4 address")
    elif bitwidth == 48:
        try:
            return macAddr_to_bytes(input_str)
        except CLI_FormatExploreError:
            pass
        except UIn_BadMacError:
            raise UIn_BadParamError("Invalid MAC address")
    elif bitwidth == 128:
        try:
            return ipv6Addr_to_bytes(input_str)
        except CLI_FormatExploreError:
            pass
        except UIn_BadIPv6Error:
            raise UIn_BadParamError("Invalid IPv6 address")
    try:
        input_ = int(input_str, 0)
    except:
        raise UIn_BadParamError(
            "Invalid input, could not cast to integer, try in hex with 0x prefix"
        )
    try:
        return int_to_bytes(input_, (bitwidth + 7) / 8)
    except UIn_BadParamError:
        raise

def parse_runtime_data(action, params):
    def parse_param_(field, bw):
        try:
            return parse_param(field, bw)
        except UIn_BadParamError as e:
            raise UIn_RuntimeDataError(
                "Error while parsing %s - %s" % (field, e)
            )

    bitwidths = [bw for( _, bw) in action.runtime_data]
    byte_array = []
    for input_str, bitwidth in zip(params, bitwidths):
        byte_array += [bytes_to_string(parse_param_(input_str, bitwidth))]
    return byte_array

_match_types_mapping = {
    MatchType.EXACT : BmMatchParamType.EXACT,
    MatchType.LPM : BmMatchParamType.LPM,
    MatchType.TERNARY : BmMatchParamType.TERNARY,
    MatchType.VALID : BmMatchParamType.VALID,
}

def parse_match_key(table, key_fields):

    def parse_param_(field, bw):
        try:
            return parse_param(field, bw)
        except UIn_BadParamError as e:
            raise UIn_MatchKeyError(
                "Error while parsing %s - %s" % (field, e)
            )

    params = []
    match_types = [t for (_, t, _) in table.key]
    bitwidths = [bw for (_, _, bw) in table.key]
    for idx, field in enumerate(key_fields):
        param_type = _match_types_mapping[match_types[idx]]
        bw = bitwidths[idx]
        if param_type == BmMatchParamType.EXACT:
            key = bytes_to_string(parse_param_(field, bw))
            param = BmMatchParam(type = param_type,
                                 exact = BmMatchParamExact(key))
        elif param_type == BmMatchParamType.LPM:
            prefix, length = field.split("/")
            key = bytes_to_string(parse_param_(prefix, bw))
            param = BmMatchParam(type = param_type,
                                 lpm = BmMatchParamLPM(key, int(length)))
        elif param_type == BmMatchParamType.TERNARY:
            key, mask = field.split("&&&")
            key = bytes_to_string(parse_param_(key, bw))
            mask = bytes_to_string(parse_param_(mask, bw))
            if len(mask) != len(key):
                raise UIn_MatchKeyError(
                    "Key and mask have different lengths in expression %s" % field
                )
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

def handle_bad_input(f):
    @wraps(f)
    def handle(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except UIn_MatchKeyError as e:
            print "Invalid match key:", e
        except UIn_RuntimeDataError as e:
            print "Invalid runtime data:", e
        except UIn_Error as e:
            print "Error:", e
        except InvalidTableOperation as e:
            error = TableOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid table operation (%s)" % error
        except InvalidCounterOperation as e:
            error = CounterOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid counter operation (%s)" % error
        except InvalidMeterOperation as e:
            error = MeterOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid meter operation (%s)" % error
        except InvalidRegisterOperation as e:
            error = RegisterOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid meter operation (%s)" % error
        except InvalidSwapOperation as e:
            error = SwapOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid swap operation (%s)" % error
    return handle

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

    def get_res(self, type_name, name, array):
        if name not in array:
            raise UIn_ResourceError(type_name, name)
        return array[name]

    def at_least_n_args(self, args, n):
        if len(args) < n:
            raise UIn_Error("Insufficient number of args")

    def exactly_n_args(self, args, n):
        if len(args) != n:
            raise UIn_Error(
                "Wrong number of args, expected %d but got %d" % (n, len(args))
            )

    def _complete_res(self, array, text):
        res = sorted(array.keys())
        if not text:
            return res
        return [r for r in res if r.startswith(text)]

    def do_show_tables(self, line):
        "List tables defined in the P4 program"
        for table_name in sorted(TABLES):
            print TABLES[table_name].table_str()

    def do_show_actions(self, line):
        "List tables defined in the P4 program"
        for action_name in sorted(ACTIONS):
            print ACTIONS[action_name].action_str()

    def _complete_tables(self, text):
        return self._complete_res(TABLES, text)

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

    # for debugging
    def print_set_default(self, table_name, action_name, runtime_data):
        print "Setting default action of", table_name
        print "{0:20} {1}".format("action:", action_name)
        print "{0:20} {1}".format(
            "runtime data:",
            "\t".join(printable_byte_str(d) for d in runtime_data)
        )

    @handle_bad_input
    def do_table_set_default(self, line):
        "Set default action for a match table: table_set_default <table name> <action name> <action parameters>"
        args = line.split()

        self.at_least_n_args(args, 2)

        table_name, action_name = args[0], args[1]

        table = self.get_res("table", table_name, TABLES)
        if action_name not in table.actions:
            raise UIn_Error(
                "Table %s has no action %s" % (table_name, action_name)
            )
        action = ACTIONS[action_name]
        if len(args[2:]) != action.num_params():
            raise UIn_Error(
                "Action %s needs %d parameters" % (action_name, action.num_params())
            )

        runtime_data = parse_runtime_data(action, args[2:])

        self.print_set_default(table_name, action_name, runtime_data)

        self.client.bm_mt_set_default_action(table_name, action_name, runtime_data)

    def complete_table_set_default(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    def parse_runtime_data(self, action, action_params):
        if len(action_params) != action.num_params():
            raise UIn_Error(
                "Action %s needs %d parameters" % (action.name, action.num_params())
            )

        return parse_runtime_data(action, action_params)

    # for debugging
    def print_table_add(self, match_key, action_name, runtime_data):
        print "{0:20} {1}".format(
            "match key:",
            "\t".join(d.to_str() for d in match_key)
        )
        print "{0:20} {1}".format("action:", action_name)
        print "{0:20} {1}".format(
            "runtime data:",
            "\t".join(printable_byte_str(d) for d in runtime_data)
        )

    @handle_bad_input
    def do_table_add(self, line):
        "Add entry to a match table: table_add <table name> <action name> <match fields> => <action parameters> [priority]"
        args = line.split()

        self.at_least_n_args(args, 3)

        table_name, action_name = args[0], args[1]
        table = self.get_res("table", table_name, TABLES)
        if action_name not in table.actions:
            raise UIn_Error(
                "Table %s has no action %s" % (table_name, action_name)
            )

        if table.match_type == MatchType.TERNARY:
            try:
                priority = int(args.pop(-1))
            except:
                raise UIn_Error(
                    "Table is ternary, but could not extract a valid priority from args"
                )
        else:
            priority = 0

        # guaranteed to exist
        action = ACTIONS[action_name]

        for idx, input_ in enumerate(args[2:]):
            if input_ == "=>": break
        idx += 2
        match_key = args[2:idx]
        action_params = args[idx+1:]
        if len(match_key) != table.num_key_fields():
            raise UIn_Error(
                "Table %s needs %d key fields" % (table_name, table.num_key_fields())
            )

        runtime_data = self.parse_runtime_data(action, action_params)

        match_key = parse_match_key(table, match_key)

        print "Adding entry to", MatchType.to_str(table.match_type), "match table", table_name

        # disable, maybe a verbose CLI option?
        self.print_table_add(match_key, action_name, runtime_data)

        entry_handle = self.client.bm_mt_add_entry(
            table_name, match_key, action_name, runtime_data,
            BmAddEntryOptions(priority = priority)
        )

        print "Entry has been added with handle", entry_handle

    def complete_table_add(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    @handle_bad_input
    def do_table_modify(self, line):
        "Add entry to a match table: table_modify <table name> <action name> <entry handle> [action parameters]"
        args = line.split()

        self.at_least_n_args(args, 3)

        table_name, action_name = args[0], args[1]
        table = self.get_res("table", table_name, TABLES)
        if action_name not in table.actions:
            raise UIn_Error(
                "Table %s has no action %s" % (table_name, action_name)
            )

        # guaranteed to exist
        action = ACTIONS[action_name]

        try:
            entry_handle = int(args[2])
        except:
            raise UIn_Error("Bad format for entry handle")

        action_params = args[3:]
        if args[3] == "=>":
            # be more tolerant
            action_params = args[4:]
        runtime_data = self.parse_runtime_data(action, action_params)

        print "Modifying entry", entry_handle, "for", MatchType.to_str(table.match_type), "match table", table_name

        entry_handle = self.client.bm_mt_modify_entry(
            table_name, entry_handle, action_name, runtime_data
        )

    def complete_table_modify(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    @handle_bad_input
    def do_table_delete(self, line):
        "Delete entry from a match table: table_delete <table name> <entry handle>"
        args = line.split()

        self.exactly_n_args(args, 2)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        try:
            entry_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for entry handle")

        print "Deleting entry", entry_handle, "from", table_name

        self.client.bm_mt_delete_entry(table_name, entry_handle)

    def complete_table_delete(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    def check_indirect(self, table):
        if table.type_ not in {TableType.indirect, TableType.indirect_ws}:
            raise UIn_Error("Cannot run this command on non-indirect table")

    def check_indirect_ws(self, table):
        if table.type_ != TableType.indirect_ws:
            raise UIn_Error(
                "Cannot run this command on non-indirect table,"\
                " or on indirect table with no selector"
            )

    @handle_bad_input
    def do_table_indirect_create_member(self, line):
        "Add a member to an indirect match table: table_indirect_create_member <table name> <action_name> [action parameters]"
        args = line.split()

        self.at_least_n_args(args, 2)

        table_name, action_name = args[0], args[1]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect(table)

        if action_name not in table.actions:
            raise UIn_Error(
                "Table %s has no action %s" % (table_name, action_name)
            )
        action = ACTIONS[action_name]

        action_params = args[2:]
        runtime_data = self.parse_runtime_data(action, action_params)

        mbr_handle = self.client.bm_mt_indirect_add_member(
            table_name, action_name, runtime_data
        )

        print "Member has been created with handle", mbr_handle

    def complete_table_indirect_create_member(self, text, line, start_index, end_index):
        # TODO: only show indirect tables
        return self._complete_table_and_action(text, line)

    @handle_bad_input
    def do_table_indirect_delete_member(self, line):
        "Delete a member in an indirect match table: table_indirect_delete_member <table name> <member handle>"
        args = line.split()

        self.exactly_n_args(args, 2)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect(table)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        self.client.bm_mt_indirect_delete_member(table_name, mbr_handle)

    def complete_table_indirect_delete_member(self, text, line, start_index, end_index):
        # TODO: only show indirect tables
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_modify_member(self, line):
        "Modify member in an indirect match table: table_indirect_modify_member <table name> <action_name> <member_handle> [action parameters]"
        args = line.split()

        self.at_least_n_args(args, 3)

        table_name, action_name = args[0], args[1]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect(table)

        if action_name not in table.actions:
            raise UIn_Error(
                "Table %s has no action %s" % (table_name, action_name)
            )
        action = ACTIONS[action_name]

        try:
            mbr_handle = int(args[2])
        except:
            raise UIn_Error("Bad format for member handle")

        action_params = args[3:]
        if args[3] == "=>":
            # be more tolerant
            action_params = args[4:]
        runtime_data = self.parse_runtime_data(action, action_params)

        mbr_handle = self.client.bm_mt_indirect_modify_member(
            table_name, action_name, mbr_handle, runtime_data
        )

    def complete_table_indirect_modify_member(self, text, line, start_index, end_index):
        # TODO: only show indirect tables
        return self._complete_table_and_action(text, line)

    def indirect_add_common(self, line, ws=False):
        args = line.split()

        self.at_least_n_args(args, 2)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        if ws:
            self.check_indirect_ws(table)
        else:
            self.check_indirect(table)

        if table.match_type == MatchType.TERNARY:
            try:
                priority = int(args.pop(-1))
            except:
                raise UIn_Error(
                    "Table is ternary, but could not extract a valid priority from args"
                )
        else:
            priority = 0

        for idx, input_ in enumerate(args[1:]):
            if input_ == "=>": break
        idx += 1
        match_key = args[1:idx]
        if len(args) != (idx + 2):
            raise UIn_Error("Invalid arguments, could not find handle")
        handle = args[idx+1]

        try:
            handle = int(handle)
        except:
            raise UIn_Error("Bad format for handle")

        match_key = parse_match_key(table, match_key)

        print "Adding entry to indirect match table", table_name

        return table_name, match_key, handle, BmAddEntryOptions(priority = priority)

    @handle_bad_input
    def do_table_indirect_add(self, line):
        "Add entry to an indirect match table: table_indirect_add <table name> <match fields> => <member handle> [priority]"

        table_name, match_key, handle, options = self.indirect_add_common(line)

        entry_handle = self.client.bm_mt_indirect_add_entry(
            table_name, match_key, handle, options
        )

        print "Entry has been added with handle", entry_handle

    def complete_table_indirect_add(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_add_with_group(self, line):
        "Add entry to an indirect match table: table_indirect_add <table name> <match fields> => <group handle> [priority]"

        table_name, match_key, handle, options = self.indirect_add_common(line, ws=True)

        entry_handle = self.client.bm_mt_indirect_ws_add_entry(
            table_name, match_key, handle, options
        )

        print "Entry has been added with handle", entry_handle

    def complete_table_indirect_add_with_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_delete(self, line):
        "Delete entry from an indirect match table: table_indirect_delete <table name> <entry handle>"
        args = line.split()

        self.exactly_n_args(args, 2)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)
        self.check_indirect(table)

        try:
            entry_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for entry handle")

        print "Deleting entry", entry_handle, "from", table_name

        self.client.bm_mt_indirect_delete_entry(table_name, entry_handle)

    def complete_table_indirect_delete(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    def indirect_set_default_common(self, line, ws=False):
        args = line.split()

        self.exactly_n_args(args, 2)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        if ws:
            self.check_indirect_ws(table)
        else:
            self.check_indirect(table)

        try:
            handle = int(args[1])
        except:
            raise UIn_Error("Bad format for handle")

        return table_name, handle

    @handle_bad_input
    def do_table_indirect_set_default(self, line):
        "Set default member for indirect match table: table_indirect_set_default <table name> <member handle>"

        table_name, handle = self.indirect_set_default_common(line)

        self.client.bm_mt_indirect_set_default_member(table_name, handle)

    def complete_table_indirect_set_default(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_set_default_with_group(self, line):
        "Set default group for indirect match table: table_indirect_set_default <table name> <group handle>"

        table_name, handle = self.indirect_set_default_common(line, ws=True)

        self.client.bm_mt_indirect_ws_set_default_group(table_name, handle)

    def complete_table_indirect_set_default_with_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_create_group(self, line):
        "Add a group to an indirect match table: table_indirect_create_group <table name>"
        args = line.split()

        self.exactly_n_args(args, 1)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect_ws(table)

        grp_handle = self.client.bm_mt_indirect_ws_create_group(table_name)

        print "Group has been created with handle", grp_handle

    def complete_table_indirect_create_group(self, text, line, start_index, end_index):
        # TODO: only show indirect_ws tables
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_delete_group(self, line):
        "Delete a group: table_indirect_delete_group <table name> <group handle>"
        args = line.split()

        self.exactly_n_args(args, 2)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect_ws(table)

        try:
            grp_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for group handle")

        self.client.bm_mt_indirect_ws_delete_group(table_name, grp_handle)

    def complete_table_indirect_delete_group(self, text, line, start_index, end_index):
        # TODO: only show indirect_ws tables
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_add_member_to_group(self, line):
        "Delete a group: table_indirect_add_member_to_group <table name> <member handle> <group handle>"
        args = line.split()

        self.exactly_n_args(args, 3)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect_ws(table)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        try:
            grp_handle = int(args[2])
        except:
            raise UIn_Error("Bad format for group handle")

        self.client.bm_mt_indirect_ws_add_member_to_group(
            table_name, mbr_handle, grp_handle
        )

    def complete_table_indirect_add_member_to_group(self, text, line, start_index, end_index):
        # TODO: only show indirect_ws tables
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_remove_member_from_group(self, line):
        "Delete a group: table_indirect_remove_member_from_group <table name> <member handle> <group handle>"
        args = line.split()

        self.exactly_n_args(args, 3)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        self.check_indirect_ws(table)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        try:
            grp_handle = int(args[2])
        except:
            raise UIn_Error("Bad format for group handle")

        self.client.bm_mt_indirect_ws_remove_member_from_group(
            table_name, mbr_handle, grp_handle
        )

    def complete_table_indirect_remove_member_from_group(self, text, line, start_index, end_index):
        # TODO: only show indirect_ws tables
        return self._complete_tables(text)


    def check_has_pre(self):
        if self.pre_type == PreType.None:
            raise UIn_Error(
                "Cannot execute this command without packet replication engine"
            )

    def get_mgrp(self, s):
        try:
            return int(s)
        except:
            raise UIn_Error("Bad format for multicast group id")

    @handle_bad_input
    def do_mc_mgrp_create(self, line):
        "Create multicast group: mc_mgrp_create <group id>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 1)
        mgrp = self.get_mgrp(args[0])
        print "Creating multicast group", mgrp
        mgrp_hdl = self.mc_client.bm_mc_mgrp_create(mgrp)
        assert(mgrp == mgrp_hdl)

    @handle_bad_input
    def do_mc_mgrp_destroy(self, line):
        "Destroy multicast group: mc_mgrp_destroy <group id>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 1)
        mgrp = self.get_mgrp(args[0])
        print "Destroying multicast group", mgrp
        self.mc_client.bm_mc_mgrp_destroy(mgrp)

    def ports_to_port_map_str(self, ports):
        last_port_num = 0
        port_map_str = ""
        ports_int = []
        for port_num_str in ports:
            try:
                port_num = int(port_num_str)
            except:
                raise UIn_Error("'%s' is not a valid port number" % port_num_str)
            ports_int.append(port_num)
        ports_int.sort()
        for port_num in ports_int:
            port_map_str += "0" * (port_num - last_port_num) + "1"
            last_port_num = port_num + 1
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

    @handle_bad_input
    def do_mc_node_create(self, line):
        "Create multicast node: mc_node_create <rid> <space-separated port list> [ | <space-separated lag list> ]"
        self.check_has_pre()
        args = line.split()
        self.at_least_n_args(args, 1)
        try:
            rid = int(args[0])
        except:
            raise UIn_Error("Bad format for rid")
        port_map_str, lag_map_str = self.parse_ports_and_lags(args)
        if self.pre_type == PreType.SimplePre:
            print "Creating node with rid", rid, "and with port map", port_map_str
            l1_hdl = self.mc_client.bm_mc_node_create(rid, port_map_str)
        else:
            print "Creating node with rid", rid, ", port map", port_map_str, "and lag map", lag_map_str
            l1_hdl = self.mc_client.bm_mc_node_create(rid, port_map_str, lag_map_str)
        print "node was created with handle", l1_hdl

    def get_node_handle(self, s):
        try:
            return int(s)
        except:
            raise UIn_Error("Bad format for node handle")

    @handle_bad_input
    def do_mc_node_update(self, line):
        "Update multicast node: mc_node_update <node handle> <space-separated port list> [ | <space-separated lag list> ]"
        self.check_has_pre()
        args = line.split()
        self.at_least_n_args(args, 2)
        l1_hdl = self.get_node_handle(args[0])
        port_map_str, lag_map_str = self.parse_ports_and_lags(args)
        if self.pre_type == PreType.SimplePre:
            print "Updating node", l1_hdl, "with port map", port_map_str
            self.mc_client.bm_mc_node_update(l1_hdl, port_map_str)
        else:
            print "Updating node", l1_hdl, "with port map", port_map_str, "and lag map", lag_map_str
            self.mc_client.bm_mc_node_update(l1_hdl, port_map_str, lag_map_str)

    @handle_bad_input
    def do_mc_node_associate(self, line):
        "Associate node to multicast group: mc_node_associate <group handle> <node handle>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 2)
        mgrp = self.get_mgrp(args[0])
        l1_hdl = self.get_node_handle(args[1])
        print "Associating node", l1_hdl, "to multicast group", mgrp
        self.mc_client.bm_mc_node_associate(mgrp, l1_hdl)

    @handle_bad_input
    def do_mc_node_dissociate(self, line):
        "Dissociate node from multicast group: mc_node_associate <group handle> <node handle>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 2)
        mgrp = self.get_mgrp(args[0])
        l1_hdl = self.get_node_handle(args[1])
        print "Dissociating node", l1_hdl, "from multicast group", mgrp
        self.mc_client.bm_mc_node_dissociate(mgrp, l1_hdl)

    @handle_bad_input
    def do_mc_node_destroy(self, line):
        "Destroy multicast node: mc_node_destroy <node handle>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 1)
        l1_hdl = int(line.split()[0])
        print "Destroying node", l1_hdl
        self.mc_client.bm_mc_node_destroy(l1_hdl)

    @handle_bad_input
    def do_mc_set_lag_membership(self, line):
        "Set lag membership of port list: mc_set_lag_membership <lag index> <space-separated port list>"
        self.check_has_pre()
        if self.pre_type != PreType.SimplePreLAG:
            raise UIn_Error(
                "Cannot execute this command with this type of PRE,"\
                " SimplePreLAG is required"
            )
        args = line.split()
        self.at_least_n_args(args, 2)
        try:
            lag_index = int(args[0])
        except:
            raise UIn_Error("Bad format for lag index")
        port_map_str = self.ports_to_port_map_str(args[1:])
        print "Setting lag membership:", lag_index, "<-", port_map_str
        self.mc_client.bm_mc_set_lag_membership(lag_index, port_map_str)

    @handle_bad_input
    def do_load_new_config_file(self, line):
        "Load new json config: load_new_config_file <path to .json file>"
        filename = line
        if not os.path.isfile(filename):
            raise UIn_Error("Not a valid filename")
        print "Loading new Json config"
        with open(filename, 'r') as f:
            json_str = f.read()
            try:
                json.loads(json_str)
            except:
                raise UIn_Error("Not a valid JSON file")
            self.client.bm_load_new_config(json_str)

    @handle_bad_input
    def do_swap_configs(self, line):
        "Swap the 2 existing configs, need to have called load_new_config_file before"
        print "Swapping configs"
        self.client.bm_swap_configs()

    @handle_bad_input
    def do_meter_set_rates(self, line):
        "Configure rates for meter: meter_set_rates <name> <rate_1>:<burst_1> <rate_2>:<burst_2> ..."
        args = line.split()
        self.at_least_n_args(args, 1)
        meter_name = args[0]
        meter = self.get_res("meter", meter_name, METER_ARRAYS)
        rates = args[1:]
        if len(rates) != meter.rate_count:
            raise UIn_Error(
                "Invalid number of rates, expected %d but got %d"\
                % (meter.rate_count, len(rates))
            )
        new_rates = []
        for rate in rates:
            try:
                r, b = rate.split(':')
                r = float(r)
                b = int(b)
                new_rates.append(BmMeterRateConfig(r, b))
            except:
                raise UIn_Error("Error while parsing rates")
        self.client.bm_meter_array_set_rates(meter_name, new_rates)

    def complete_meter_set_rates(self, text, line, start_index, end_index):
        return self._complete_meters(text)

    def _complete_meters(self, text):
        return self._complete_res(METER_ARRAYS, text)

    @handle_bad_input
    def do_counter_read(self, line):
        "Read counter value: counter_read <name> <index>"
        args = line.split()
        self.exactly_n_args(args, 2)
        counter_name = args[0]
        counter = self.get_res("counter", counter_name, COUNTER_ARRAYS)
        index = args[1]
        try:
            index = int(index)
        except:
            raise UIn_Error("Bad format for index")
        if counter.is_direct:
            table_name = counter.binding
            print "this is the direct counter for table", table_name
            index = index & 0xffffffff
            value = self.client.bm_mt_read_counter(table_name, index)
        else:
            value = self.client.bm_counter_read(counter_name, index)
        print "%s[%d]= " % (counter_name, index), value

    def complete_counter_read(self, text, line, start_index, end_index):
        return self._complete_counters(text)

    @handle_bad_input
    def do_counter_reset(self, line):
        "Reset counter: counter_reset <name>"
        args = line.split()
        self.exactly_n_args(args, 1)
        counter_name = args[0]
        counter = self.get_res("counter", counter_name, COUNTER_ARRAYS)
        if counter.is_direct:
            table_name = counter.binding
            print "this is the direct counter for table", table_name
            value = self.client.bm_mt_reset_counters(table_name)
        else:
            value = self.client.bm_counter_reset_all(counter_name)

    def complete_counter_reset(self, text, line, start_index, end_index):
        return self._complete_counters(text)

    def _complete_counters(self, text):
        return self._complete_res(COUNTER_ARRAYS, text)

    @handle_bad_input
    def do_register_read(self, line):
        "Read register value: register_read <name> <index>"
        args = line.split()
        self.exactly_n_args(args, 2)
        register_name = args[0]
        register = self.get_res("register", register_name, REGISTER_ARRAYS)
        index = args[1]
        try:
            index = int(index)
        except:
            raise UIn_Error("Bad format for index")
        value = self.client.bm_register_read(register_name, index)
        print "%s[%d]= " % (register_name, index), value

    def complete_register_read(self, text, line, start_index, end_index):
        return self._complete_registers(text)

    @handle_bad_input
    def do_register_write(self, line):
        "Write register value: register_write <name> <index> <value>"
        args = line.split()
        self.exactly_n_args(args, 3)
        register_name = args[0]
        register = self.get_res("register", register_name, REGISTER_ARRAYS)
        index = args[1]
        try:
            index = int(index)
        except:
            raise UIn_Error("Bad format for index")
        value = args[2]
        try:
            value = int(value)
        except:
            raise UIn_Error("Bad format for value, must be an integer")
        self.client.bm_register_write(register_name, index, value)

    def complete_register_write(self, text, line, start_index, end_index):
        return self._complete_registers(text)

    def _complete_registers(self, text):
        return self._complete_res(REGISTER_ARRAYS, text)

    @handle_bad_input
    def do_table_dump(self, line):
        "Display some (non-formatted) information about a table: table_dump <table_name>"
        args = line.split()
        self.exactly_n_args(args, 1)
        table_name = args[0]
        self.get_res("table", table_name, TABLES)
        print self.client.bm_dump_table(table_name)

    def complete_table_dump(self, text, line, start_index, end_index):
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

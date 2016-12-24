#!/usr/bin/env python2

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
import bmpy_utils as utils

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
                        type=str, action="store", required=False)

    parser.add_argument('--pre', help='Packet Replication Engine used by target',
                        type=str, choices=['None', 'SimplePre', 'SimplePreLAG'],
                        default=PreType.SimplePre, action=ActionToPreType)

    return parser

TABLES = {}
ACTION_PROFS = {}
ACTIONS = {}
METER_ARRAYS = {}
COUNTER_ARRAYS = {}
REGISTER_ARRAYS = {}
CUSTOM_CRC_CALCS = {}

class MatchType:
    EXACT = 0
    LPM = 1
    TERNARY = 2
    VALID = 3
    RANGE = 4

    @staticmethod
    def to_str(x):
        return {0: "exact", 1: "lpm", 2: "ternary", 3: "valid", 4: "range"}[x]

    @staticmethod
    def from_str(x):
        return {"exact": 0, "lpm": 1, "ternary": 2, "valid": 3, "range": 4}[x]

class Table:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.match_type_ = None
        self.actions = {}
        self.key = []
        self.default_action = None
        self.type_ = None
        self.support_timeout = False
        self.action_prof = None

        TABLES[name] = self

    def num_key_fields(self):
        return len(self.key)

    def key_str(self):
        return ",\t".join([name + "(" + MatchType.to_str(t) + ", " + str(bw) + ")" for name, t, bw in self.key])

    def table_str(self):
        ap_str = "implementation={}".format(
            "None" if not self.action_prof else self.action_prof.name)
        return "{0:30} [{1}, mk={2}]".format(self.name, ap_str, self.key_str())

class ActionProf:
    def __init__(self, name, id_):
        self.name = name
        self.id_ = id_
        self.with_selection = False
        self.actions = {}
        self.ref_cnt = 0

        ACTION_PROFS[name] = self

    def action_prof_str(self):
        return "{0:30} [{1}]".format(self.name, self.with_selection)

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
        self.is_direct = None
        self.size = None
        self.binding = None
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

def reset_config():
    TABLES.clear()
    ACTION_PROFS.clear()
    ACTIONS.clear()
    METER_ARRAYS.clear()
    COUNTER_ARRAYS.clear()
    REGISTER_ARRAYS.clear()
    CUSTOM_CRC_CALCS.clear()

def load_json_str(json_str):
    def get_header_type(header_name, j_headers):
        for h in j_headers:
            if h["name"] == header_name:
                return h["header_type"]
        assert(0)

    def get_field_bitwidth(header_type, field_name, j_header_types):
        for h in j_header_types:
            if h["name"] != header_type: continue
            for t in h["fields"]:
                # t can have a third element (field signedness)
                f, bw = t[0], t[1]
                if f == field_name:
                    return bw
        assert(0)

    reset_config()
    json_ = json.loads(json_str)

    for j_action in json_["actions"]:
        action = Action(j_action["name"], j_action["id"])
        for j_param in j_action["runtime_data"]:
            action.runtime_data += [(j_param["name"], j_param["bitwidth"])]

    for j_pipeline in json_["pipelines"]:
        if "action_profiles" in j_pipeline:  # new JSON format
            for j_aprof in j_pipeline["action_profiles"]:
                action_prof = ActionProf(j_aprof["name"], j_aprof["id"])
                action_prof.with_selection = "selector" in j_aprof

        for j_table in j_pipeline["tables"]:
            table = Table(j_table["name"], j_table["id"])
            table.match_type = MatchType.from_str(j_table["match_type"])
            table.type_ = TableType.from_str(j_table["type"])
            table.support_timeout = j_table["support_timeout"]
            for action in j_table["actions"]:
                table.actions[action] = ACTIONS[action]

            if table.type_ in {TableType.indirect, TableType.indirect_ws}:
                if "action_profile" in j_table:
                    action_prof = ACTION_PROFS[j_table["action_profile"]]
                else:  # for backward compatibility
                    assert("act_prof_name" in j_table)
                    action_prof = ActionProf(j_table["act_prof_name"],
                                             table.id_)
                    action_prof.with_selection = "selector" in j_table
                action_prof.actions.update(table.actions)
                action_prof.ref_cnt += 1
                table.action_prof = action_prof

            for j_key in j_table["key"]:
                target = j_key["target"]
                match_type = MatchType.from_str(j_key["match_type"])
                if match_type == MatchType.VALID:
                    field_name = target + "_valid"
                    bitwidth = 1
                elif target[1] == "$valid$":
                    field_name = target[0] + "_valid"
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
            if "is_direct" in j_meter and j_meter["is_direct"]:
                meter_array.is_direct = True
                meter_array.binding = j_meter["binding"]
            else:
                meter_array.is_direct = False
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

    if "calculations" in json_:
        for j_calc in json_["calculations"]:
            calc_name = j_calc["name"]
            if j_calc["algo"] == "crc16_custom":
                CUSTOM_CRC_CALCS[calc_name] = 16
            elif j_calc["algo"] == "crc32_custom":
                CUSTOM_CRC_CALCS[calc_name] = 32

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
    MatchType.RANGE : BmMatchParamType.RANGE,
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
        elif param_type == BmMatchParamType.RANGE:
            start, end = field.split("->")
            start = bytes_to_string(parse_param_(start, bw))
            end = bytes_to_string(parse_param_(end, bw))
            if len(start) != len(end):
                raise UIn_MatchKeyError(
                    "start and end have different lengths in expression %s" % field
                )
            if start > end:
                raise UIn_MatchKeyError(
                    "start is less than end in expression %s" % field
                )
            param = BmMatchParam(type = param_type,
                                 range = BmMatchParamRange(start, end))
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
        (self.valid.to_str() if self.valid else "") +\
        (self.range.to_str() if self.range else "")

def BmMatchParamExact_to_str(self):
    return printable_byte_str(self.key)

def BmMatchParamLPM_to_str(self):
    return printable_byte_str(self.key) + "/" + str(self.prefix_length)

def BmMatchParamTernary_to_str(self):
    return printable_byte_str(self.key) + " &&& " + printable_byte_str(self.mask)

def BmMatchParamValid_to_str(self):
    return ""

def BmMatchParamRange_to_str(self):
    return printable_byte_str(self.start) + " -> " + printable_byte_str(self.end_)

BmMatchParam.to_str = BmMatchParam_to_str
BmMatchParamExact.to_str = BmMatchParamExact_to_str
BmMatchParamLPM.to_str = BmMatchParamLPM_to_str
BmMatchParamTernary.to_str = BmMatchParamTernary_to_str
BmMatchParamValid.to_str = BmMatchParamValid_to_str
BmMatchParamRange.to_str = BmMatchParamRange_to_str

# services is [(service_name, client_class), ...]
def thrift_connect(thrift_ip, thrift_port, services):
    return utils.thrift_connect(thrift_ip, thrift_port, services)

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
            print "Invalid register operation (%s)" % error
        except InvalidLearnOperation as e:
            error = LearnOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid learn operation (%s)" % error
        except InvalidSwapOperation as e:
            error = SwapOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid swap operation (%s)" % error
        except InvalidDevMgrOperation as e:
            error = DevMgrErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid device manager operation (%s)" % error
        except InvalidCrcOperation as e:
            error = CrcErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid crc operation (%s)" % error
    return handle

def deprecated_act_prof(substitute, with_selection=False,
                        strictly_deprecated=True):
    # need two levels here because our decorator takes arguments
    def deprecated_act_prof_(f):
        # not sure if this is the right place for it, if I want it to play nice
        # with @wraps
        if strictly_deprecated:
            f.__doc__ = "[DEPRECATED!] " + f.__doc__
            f.__doc__ += "\nUse '{}' instead".format(substitute)
        @wraps(f)
        def wrapper(obj, line):
            substitute_fn = getattr(obj, "do_" + substitute)
            args = line.split()
            obj.at_least_n_args(args, 1)
            table_name = args[0]
            table = obj.get_res("table", table_name, TABLES)
            if with_selection:
                obj.check_indirect_ws(table)
            else:
                obj.check_indirect(table)
            assert(table.action_prof is not None)
            assert(table.action_prof.ref_cnt > 0)
            if strictly_deprecated and table.action_prof.ref_cnt > 1:
                raise UIn_Error(
                    "Legacy command does not work with shared action profiles")
            args[0] = table.action_prof.name
            if strictly_deprecated:
                # writing to stderr in case someone is parsing stdout
                sys.stderr.write(
                    "This is a deprecated command, use '{}' instead\n".format(
                        substitute))
            return substitute_fn(" ".join(args))
        # we add the handle_bad_input decorator "programatically"
        return handle_bad_input(wrapper)
    return deprecated_act_prof_

# thrift does not support unsigned integers
def hex_to_i16(h):
    x = int(h, 0)
    if (x > 0xFFFF):
        raise UIn_Error("Integer cannot fit within 16 bits")
    if (x > 0x7FFF): x-= 0x10000
    return x
def i16_to_hex(h):
    x = int(h)
    if (x & 0x8000): x+= 0x10000
    return x
def hex_to_i32(h):
    x = int(h, 0)
    if (x > 0xFFFFFFFF):
        raise UIn_Error("Integer cannot fit within 32 bits")
    if (x > 0x7FFFFFFF): x-= 0x100000000
    return x
def i32_to_hex(h):
    x = int(h)
    if (x & 0x80000000): x+= 0x100000000
    return x

def parse_bool(s):
    if s == "true" or s == "True":
        return True
    if s == "false" or s  == "False":
        return False
    try:
        s = int(s, 0)
        return bool(s)
    except:
        pass
    raise UIn_Error("Invalid bool parameter")

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

    @handle_bad_input
    def do_show_tables(self, line):
        "List tables defined in the P4 program: show_tables"
        self.exactly_n_args(line.split(), 0)
        for table_name in sorted(TABLES):
            print TABLES[table_name].table_str()

    @handle_bad_input
    def do_show_actions(self, line):
        "List actions defined in the P4 program: show_actions"
        self.exactly_n_args(line.split(), 0)
        for action_name in sorted(ACTIONS):
            print ACTIONS[action_name].action_str()

    def _complete_tables(self, text):
        return self._complete_res(TABLES, text)

    def _complete_act_profs(self, text):
        return self._complete_res(ACTION_PROFS, text)

    @handle_bad_input
    def do_table_show_actions(self, line):
        "List one table's actions as per the P4 program: table_show_actions <table_name>"
        self.exactly_n_args(line.split(), 1)
        table = TABLES[line]
        for action_name in sorted(table.actions):
            print ACTIONS[action_name].action_str()

    def complete_table_show_actions(self, text, line, start_index, end_index):
        return self._complete_tables(self, text)

    @handle_bad_input
    def do_table_info(self, line):
        "Show info about a table: table_info <table_name>"
        self.exactly_n_args(line.split(), 1)
        table = TABLES[line]
        print table.table_str()
        print "*" * 80
        for action_name in sorted(table.actions):
            print ACTIONS[action_name].action_str()

    def complete_table_info(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    # used for tables but also for action profiles
    def _complete_actions(self, text, table_name = None, res = TABLES):
        if not table_name:
            actions = sorted(ACTIONS.keys())
        elif table_name not in res:
            return []
        actions = sorted(res[table_name].actions.keys())
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

    def _complete_act_prof_and_action(self, text, line):
        act_profs = sorted(ACTION_PROFS.keys())
        args = line.split()
        args_cnt = len(args)
        if args_cnt == 1 and not text:
            return self._complete_act_profs(text)
        if args_cnt == 2 and text:
            return self._complete_act_profs(text)
        act_prof_name = args[1]
        if args_cnt == 2 and not text:
            return self._complete_actions(text, act_prof_name, ACTION_PROFS)
        if args_cnt == 3 and text:
            return self._complete_actions(text, act_prof_name, ACTION_PROFS)
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

        self.client.bm_mt_set_default_action(0, table_name, action_name, runtime_data)

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
    def do_table_num_entries(self, line):
        "Return the number of entries in a match table (direct or indirect): table_num_entries <table name>"
        args = line.split()

        self.exactly_n_args(args, 1)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)

        print self.client.bm_mt_get_num_entries(0, table_name)

    def complete_table_num_entries(self, text, line, start_index, end_index):
        return self._complete_tables(text)

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

        if table.match_type in {MatchType.TERNARY, MatchType.RANGE}:
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
            0, table_name, match_key, action_name, runtime_data,
            BmAddEntryOptions(priority = priority)
        )

        print "Entry has been added with handle", entry_handle

    def complete_table_add(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    @handle_bad_input
    def do_table_set_timeout(self, line):
        "Set a timeout in ms for a given entry; the table has to support timeouts: table_set_timeout <table_name> <entry handle> <timeout (ms)>"
        args = line.split()
        self.exactly_n_args(args, 3)

        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)
        if not table.support_timeout:
            raise UIn_Error(
                "Table {} does not support entry timeouts".format(table_name))

        try:
            entry_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for entry handle")

        try:
            timeout_ms = int(args[2])
        except:
            raise UIn_Error("Bad format for timeout")

        print "Setting a", timeout_ms, "ms timeout for entry", entry_handle

        self.client.bm_mt_set_entry_ttl(0, table_name, entry_handle, timeout_ms)

    def complete_table_set_timeout(self, text, line, start_index, end_index):
        return self._complete_tables(text)

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
            0, table_name, entry_handle, action_name, runtime_data
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

        self.client.bm_mt_delete_entry(0, table_name, entry_handle)

    def complete_table_delete(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    def check_indirect(self, table):
        if table.type_ not in {TableType.indirect, TableType.indirect_ws}:
            raise UIn_Error("Cannot run this command on non-indirect table")

    def check_indirect_ws(self, table):
        if table.type_ != TableType.indirect_ws:
            raise UIn_Error(
                "Cannot run this command on non-indirect table,"\
                " or on indirect table with no selector")

    def check_act_prof_ws(self, act_prof):
        if not act_prof.with_selection:
            raise UIn_Error(
                "Cannot run this command on an action profile without selector")

    @handle_bad_input
    def do_act_prof_create_member(self, line):
        "Add a member to an action profile: act_prof_create_member <action profile name> <action_name> [action parameters]"
        args = line.split()

        self.at_least_n_args(args, 2)

        act_prof_name, action_name = args[0], args[1]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        if action_name not in act_prof.actions:
            raise UIn_Error("Action profile '{}' has no action '{}'".format(
                act_prof_name, action_name))
        action = ACTIONS[action_name]

        action_params = args[2:]
        runtime_data = self.parse_runtime_data(action, action_params)

        mbr_handle = self.client.bm_mt_act_prof_add_member(
            0, act_prof_name, action_name, runtime_data)

        print "Member has been created with handle", mbr_handle

    def complete_act_prof_create_member(self, text, line, start_index, end_index):
        return self._complete_act_prof_and_action(text, line)

    @deprecated_act_prof("act_prof_create_member")
    def do_table_indirect_create_member(self, line):
        "Add a member to an indirect match table: table_indirect_create_member <table name> <action_name> [action parameters]"
        pass

    def complete_table_indirect_create_member(self, text, line, start_index, end_index):
        return self._complete_table_and_action(text, line)

    @handle_bad_input
    def do_act_prof_delete_member(self, line):
        "Delete a member in an action profile: act_prof_delete_member <action profile name> <member handle>"
        args = line.split()

        self.exactly_n_args(args, 2)

        act_prof_name, action_name = args[0], args[1]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        self.client.bm_mt_act_prof_delete_member(0, act_prof_name, mbr_handle)

    def complete_act_prof_delete_member(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @deprecated_act_prof("act_prof_delete_member")
    def do_table_indirect_delete_member(self, line):
        "Delete a member in an indirect match table: table_indirect_delete_member <table name> <member handle>"
        pass

    def complete_table_indirect_delete_member(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_modify_member(self, line):
        "Modify member in an action profile: act_prof_modify_member <action profile name> <action_name> <member_handle> [action parameters]"
        args = line.split()

        self.at_least_n_args(args, 3)

        act_prof_name, action_name = args[0], args[1]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        if action_name not in act_prof.actions:
            raise UIn_Error("Action profile '{}' has no action '{}'".format(
                act_prof_name, action_name))
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

        mbr_handle = self.client.bm_mt_act_prof_modify_member(
            0, act_prof_name, mbr_handle, action_name, runtime_data)

    def complete_act_prof_modify_member(self, text, line, start_index, end_index):
        return self._complete_act_prof_and_action(text, line)

    @deprecated_act_prof("act_prof_modify_member")
    def do_table_indirect_modify_member(self, line):
        "Modify member in an indirect match table: table_indirect_modify_member <table name> <action_name> <member_handle> [action parameters]"
        pass

    def complete_table_indirect_modify_member(self, text, line, start_index, end_index):
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

        if table.match_type in {MatchType.TERNARY, MatchType.RANGE}:
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
            0, table_name, match_key, handle, options
        )

        print "Entry has been added with handle", entry_handle

    def complete_table_indirect_add(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_add_with_group(self, line):
        "Add entry to an indirect match table: table_indirect_add <table name> <match fields> => <group handle> [priority]"

        table_name, match_key, handle, options = self.indirect_add_common(line, ws=True)

        entry_handle = self.client.bm_mt_indirect_ws_add_entry(
            0, table_name, match_key, handle, options
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

        self.client.bm_mt_indirect_delete_entry(0, table_name, entry_handle)

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

        self.client.bm_mt_indirect_set_default_member(0, table_name, handle)

    def complete_table_indirect_set_default(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_indirect_set_default_with_group(self, line):
        "Set default group for indirect match table: table_indirect_set_default <table name> <group handle>"

        table_name, handle = self.indirect_set_default_common(line, ws=True)

        self.client.bm_mt_indirect_ws_set_default_group(0, table_name, handle)

    def complete_table_indirect_set_default_with_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_create_group(self, line):
        "Add a group to an action pofile: act_prof_create_group <action profile name>"
        args = line.split()

        self.exactly_n_args(args, 1)

        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        self.check_act_prof_ws(act_prof)

        grp_handle = self.client.bm_mt_act_prof_create_group(0, act_prof_name)

        print "Group has been created with handle", grp_handle

    def complete_act_prof_create_group(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @deprecated_act_prof("act_prof_create_group", with_selection=True)
    def do_table_indirect_create_group(self, line):
        "Add a group to an indirect match table: table_indirect_create_group <table name>"
        pass

    def complete_table_indirect_create_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_delete_group(self, line):
        "Delete a group from an action profile: act_prof_delete_group <action profile name> <group handle>"
        args = line.split()

        self.exactly_n_args(args, 2)

        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        self.check_act_prof_ws(act_prof)

        try:
            grp_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for group handle")

        self.client.bm_mt_act_prof_delete_group(0, act_prof_name, grp_handle)

    def complete_act_prof_delete_group(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @deprecated_act_prof("act_prof_delete_group", with_selection=True)
    def do_table_indirect_delete_group(self, line):
        "Delete a group: table_indirect_delete_group <table name> <group handle>"
        pass

    def complete_table_indirect_delete_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_add_member_to_group(self, line):
        "Add member to group in an action profile: act_prof_add_member_to_group <action profile name> <member handle> <group handle>"
        args = line.split()

        self.exactly_n_args(args, 3)

        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        self.check_act_prof_ws(act_prof)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        try:
            grp_handle = int(args[2])
        except:
            raise UIn_Error("Bad format for group handle")

        self.client.bm_mt_act_prof_add_member_to_group(
            0, act_prof_name, mbr_handle, grp_handle)

    def complete_act_prof_add_member_to_group(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @deprecated_act_prof("act_prof_add_member_to_group", with_selection=True)
    def do_table_indirect_add_member_to_group(self, line):
        "Add member to group: table_indirect_add_member_to_group <table name> <member handle> <group handle>"
        pass

    def complete_table_indirect_add_member_to_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_remove_member_from_group(self, line):
        "Remove member from group in action profile: act_prof_remove_member_from_group <action profile name> <member handle> <group handle>"
        args = line.split()

        self.exactly_n_args(args, 3)

        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        self.check_act_prof_ws(act_prof)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        try:
            grp_handle = int(args[2])
        except:
            raise UIn_Error("Bad format for group handle")

        self.client.bm_mt_act_prof_remove_member_from_group(
            0, act_prof_name, mbr_handle, grp_handle)

    def complete_act_prof_remove_member_from_group(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @deprecated_act_prof("act_prof_remove_member_from_group", with_selection=True)
    def do_table_indirect_remove_member_from_group(self, line):
        "Remove member from group: table_indirect_remove_member_from_group <table name> <member handle> <group handle>"
        pass

    def complete_table_indirect_remove_member_from_group(self, text, line, start_index, end_index):
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
        mgrp_hdl = self.mc_client.bm_mc_mgrp_create(0, mgrp)
        assert(mgrp == mgrp_hdl)

    @handle_bad_input
    def do_mc_mgrp_destroy(self, line):
        "Destroy multicast group: mc_mgrp_destroy <group id>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 1)
        mgrp = self.get_mgrp(args[0])
        print "Destroying multicast group", mgrp
        self.mc_client.bm_mc_mgrp_destroy(0, mgrp)

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
            l1_hdl = self.mc_client.bm_mc_node_create(0, rid, port_map_str)
        else:
            print "Creating node with rid", rid, ", port map", port_map_str, "and lag map", lag_map_str
            l1_hdl = self.mc_client.bm_mc_node_create(0, rid, port_map_str, lag_map_str)
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
            self.mc_client.bm_mc_node_update(0, l1_hdl, port_map_str)
        else:
            print "Updating node", l1_hdl, "with port map", port_map_str, "and lag map", lag_map_str
            self.mc_client.bm_mc_node_update(0, l1_hdl, port_map_str, lag_map_str)

    @handle_bad_input
    def do_mc_node_associate(self, line):
        "Associate node to multicast group: mc_node_associate <group handle> <node handle>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 2)
        mgrp = self.get_mgrp(args[0])
        l1_hdl = self.get_node_handle(args[1])
        print "Associating node", l1_hdl, "to multicast group", mgrp
        self.mc_client.bm_mc_node_associate(0, mgrp, l1_hdl)

    @handle_bad_input
    def do_mc_node_dissociate(self, line):
        "Dissociate node from multicast group: mc_node_associate <group handle> <node handle>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 2)
        mgrp = self.get_mgrp(args[0])
        l1_hdl = self.get_node_handle(args[1])
        print "Dissociating node", l1_hdl, "from multicast group", mgrp
        self.mc_client.bm_mc_node_dissociate(0, mgrp, l1_hdl)

    @handle_bad_input
    def do_mc_node_destroy(self, line):
        "Destroy multicast node: mc_node_destroy <node handle>"
        self.check_has_pre()
        args = line.split()
        self.exactly_n_args(args, 1)
        l1_hdl = int(line.split()[0])
        print "Destroying node", l1_hdl
        self.mc_client.bm_mc_node_destroy(0, l1_hdl)

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
        self.mc_client.bm_mc_set_lag_membership(0, lag_index, port_map_str)

    @handle_bad_input
    def do_mc_dump(self, line):
        "Dump entries in multicast engine"
        self.check_has_pre()
        json_dump = self.mc_client.bm_mc_get_entries(0)
        try:
            mc_json = json.loads(json_dump)
        except:
            print "Exception when retrieving MC entries"
            return

        l1_handles = {}
        for h in mc_json["l1_handles"]:
            l1_handles[h["handle"]] = (h["rid"], h["l2_handle"])
        l2_handles = {}
        for h in mc_json["l2_handles"]:
            l2_handles[h["handle"]] = (h["ports"], h["lags"])

        print "=========="
        print "MC ENTRIES"
        for mgrp in mc_json["mgrps"]:
            print "**********"
            mgid = mgrp["id"]
            print "mgrp({})".format(mgid)
            for L1h in mgrp["l1_handles"]:
                rid, L2h = l1_handles[L1h]
                print "  -> (L1h={}, rid={})".format(L1h, rid),
                ports, lags = l2_handles[L2h]
                print "-> (ports=[{}], lags=[{}])".format(
                    ", ".join([str(p) for p in ports]),
                    ", ".join([str(l) for l in lags]))

        print "=========="
        print "LAGS"
        for lag in mc_json["lags"]:
            print "lag({})".format(lag["id"]),
            print "-> ports=[{}]".format(", ".join([str(p) for p in ports]))
        print "=========="

    @handle_bad_input
    def do_load_new_config_file(self, line):
        "Load new json config: load_new_config_file <path to .json file>"
        args = line.split()
        self.exactly_n_args(args, 1)
        filename = args[0]
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
            load_json_str(json_str)

    @handle_bad_input
    def do_swap_configs(self, line):
        "Swap the 2 existing configs, need to have called load_new_config_file before"
        print "Swapping configs"
        self.client.bm_swap_configs()

    @handle_bad_input
    def do_meter_array_set_rates(self, line):
        "Configure rates for an entire meter array: meter_array_set_rates <name> <rate_1>:<burst_1> <rate_2>:<burst_2> ..."
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
        self.client.bm_meter_array_set_rates(0, meter_name, new_rates)

    def complete_meter_array_set_rates(self, text, line, start_index, end_index):
        return self._complete_meters(text)

    @handle_bad_input
    def do_meter_set_rates(self, line):
        "Configure rates for a meter: meter_set_rates <name> <index> <rate_1>:<burst_1> <rate_2>:<burst_2> ..."
        args = line.split()
        self.at_least_n_args(args, 2)
        meter_name = args[0]
        meter = self.get_res("meter", meter_name, METER_ARRAYS)
        try:
            index = int(args[1])
        except:
            raise UIn_Error("Bad format for index")
        rates = args[2:]
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
        if meter.is_direct:
            table_name = meter.binding
            self.client.bm_mt_set_meter_rates(0, table_name, index, new_rates)
        else:
            self.client.bm_meter_set_rates(0, meter_name, index, new_rates)

    def complete_meter_set_rates(self, text, line, start_index, end_index):
        return self._complete_meters(text)

    @handle_bad_input
    def do_meter_get_rates(self, line):
        "Retrieve rates for a meter: meter_get_rates <name> <index>"
        args = line.split()
        self.exactly_n_args(args, 2)
        meter_name = args[0]
        meter = self.get_res("meter", meter_name, METER_ARRAYS)
        try:
            index = int(args[1])
        except:
            raise UIn_Error("Bad format for index")
        # meter.rate_count
        if meter.is_direct:
            table_name = meter.binding
            rates = self.client.bm_mt_get_meter_rates(0, table_name, index)
        else:
            rates = self.client.bm_meter_get_rates(0, meter_name, index)
        if len(rates) != meter.rate_count:
            print "WARNING: expected", meter.rate_count, "rates",
            print "but only received", len(rates)
        for idx, rate in enumerate(rates):
            print "{}: info rate = {}, burst size = {}".format(
                idx, rate.units_per_micros, rate.burst_size)

    def complete_meter_get_rates(self, text, line, start_index, end_index):
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
            # index = index & 0xffffffff
            value = self.client.bm_mt_read_counter(0, table_name, index)
        else:
            value = self.client.bm_counter_read(0, counter_name, index)
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
            value = self.client.bm_mt_reset_counters(0, table_name)
        else:
            value = self.client.bm_counter_reset_all(0, counter_name)

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
        value = self.client.bm_register_read(0, register_name, index)
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
        self.client.bm_register_write(0, register_name, index, value)

    def complete_register_write(self, text, line, start_index, end_index):
        return self._complete_registers(text)

    @handle_bad_input
    def do_register_reset(self, line):
        "Reset all the cells in the register array to 0: register_reset <name>"
        args = line.split()
        self.exactly_n_args(args, 1)
        register_name = args[0]
        self.client.bm_register_reset(0, register_name)

    def complete_register_reset(self, text, line, start_index, end_index):
        return self._complete_registers(text)

    def _complete_registers(self, text):
        return self._complete_res(REGISTER_ARRAYS, text)

    def dump_action_and_data(self, action_name, action_data):
        def hexstr(v):
            return "".join("{:02x}".format(ord(c)) for c in v)

        print "Action entry: {} - {}".format(
            action_name, ", ".join([hexstr(a) for a in action_data]))

    def dump_action_entry(self, a_entry):
        if a_entry.action_type == BmActionEntryType.NONE:
            print "EMPTY"
        elif a_entry.action_type == BmActionEntryType.ACTION_DATA:
            self.dump_action_and_data(a_entry.action_name, a_entry.action_data)
        elif a_entry.action_type == BmActionEntryType.MBR_HANDLE:
            print "Index: member({})".format(a_entry.mbr_handle)
        elif a_entry.action_type == BmActionEntryType.GRP_HANDLE:
            print "Index: group({})".format(a_entry.grp_handle)

    def dump_one_member(self, member):
        print "Dumping member {}".format(member.mbr_handle)
        self.dump_action_and_data(member.action_name, member.action_data)

    def dump_members(self, members):
        for m in members:
            print "**********"
            self.dump_one_member(m)

    def dump_one_group(self, group):
        print "Dumping group {}".format(group.grp_handle)
        print "Members: [{}]".format(", ".join(
            [str(h) for h in group.mbr_handles]))

    def dump_groups(self, groups):
        for g in groups:
            print "**********"
            self.dump_one_group(g)

    def dump_one_entry(self, table, entry):
        if table.key:
            out_name_w = max(20, max([len(t[0]) for t in table.key]))

        def hexstr(v):
            return "".join("{:02x}".format(ord(c)) for c in v)
        def dump_exact(p):
             return hexstr(p.exact.key)
        def dump_lpm(p):
            return "{}/{}".format(hexstr(p.lpm.key), p.lpm.prefix_length)
        def dump_ternary(p):
            return "{} &&& {}".format(hexstr(p.ternary.key),
                                      hexstr(p.ternary.mask))
        def dump_range(p):
            return "{} -> {}".format(hexstr(p.range.start),
                                     hexstr(p.range.end_))
        def dump_valid(p):
            return "01" if p.valid.key else "00"
        pdumpers = {"exact": dump_exact, "lpm": dump_lpm,
                    "ternary": dump_ternary, "valid": dump_valid,
                    "range": dump_range}

        print "Dumping entry {}".format(hex(entry.entry_handle))
        print "Match key:"
        for p, k in zip(entry.match_key, table.key):
            assert(k[1] == p.type)
            pdumper = pdumpers[MatchType.to_str(p.type)]
            print "* {0:{w}}: {1:10}{2}".format(
                k[0], MatchType.to_str(p.type).upper(),
                pdumper(p), w=out_name_w)
        if entry.options.priority >= 0:
            print "Priority: {}".format(entry.options.priority)
        self.dump_action_entry(entry.action_entry)
        if entry.life is not None:
            print "Life: {}ms since hit, timeout is {}ms".format(
                entry.life.time_since_hit_ms, entry.life.timeout_ms)

    @handle_bad_input
    def do_table_dump_entry(self, line):
        "Display some information about a table entry: table_dump_entry <table name> <entry handle>"
        args = line.split()
        self.exactly_n_args(args, 2)
        table_name = args[0]

        table = self.get_res("table", table_name, TABLES)

        try:
            entry_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for entry handle")

        entry = self.client.bm_mt_get_entry(0, table_name, entry_handle)
        self.dump_one_entry(table, entry)

    def complete_table_dump_entry(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_dump_member(self, line):
        "Display some information about a member: act_prof_dump_member <action profile name> <member handle>"
        args = line.split()
        self.exactly_n_args(args, 2)

        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        try:
            mbr_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for member handle")

        member = self.client.bm_mt_act_prof_get_member(
            0, act_prof_name, mbr_handle)
        self.dump_one_member(member)

    def complete_act_prof_dump_member(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    # notice the strictly_deprecated=False; I don't consider this command to be
    # strictly deprecated because it can be convenient and does not modify the
    # action profile so won't create problems
    @deprecated_act_prof("act_prof_dump_member", with_selection=False,
                         strictly_deprecated=False)
    def do_table_dump_member(self, line):
        "Display some information about a member: table_dump_member <table name> <member handle>"
        pass

    def complete_table_dump_member(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_act_prof_dump_group(self, line):
        "Display some information about a group: table_dump_group <action profile name> <group handle>"
        args = line.split()
        self.exactly_n_args(args, 2)

        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)

        try:
            grp_handle = int(args[1])
        except:
            raise UIn_Error("Bad format for group handle")

        group = self.client.bm_mt_act_prof_get_group(
            0, act_prof_name, grp_handle)
        self.dump_one_group(group)

    def complete_act_prof_dump_group(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @deprecated_act_prof("act_prof_dump_group", with_selection=False,
                         strictly_deprecated=False)
    def do_table_dump_group(self, line):
        "Display some information about a group: table_dump_group <table name> <group handle>"
        pass

    def complete_table_dump_group(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    def _dump_act_prof(self, act_prof):
        act_prof_name = act_prof.name
        members = self.client.bm_mt_act_prof_get_members(0, act_prof_name)
        print "=========="
        print "MEMBERS"
        self.dump_members(members)
        if act_prof.with_selection:
            groups = self.client.bm_mt_act_prof_get_groups(0, act_prof_name)
            print "=========="
            print "GROUPS"
            self.dump_groups(groups)

    @handle_bad_input
    def do_act_prof_dump(self, line):
        "Display entries in an action profile: act_prof_dump <action profile name>"
        args = line.split()
        self.exactly_n_args(args, 1)
        act_prof_name = args[0]
        act_prof = self.get_res("action profile", act_prof_name, ACTION_PROFS)
        self._dump_act_prof(act_prof)

    def complete_act_prof_dump(self, text, line, start_index, end_index):
        return self._complete_act_profs(text)

    @handle_bad_input
    def do_table_dump(self, line):
        "Display entries in a match-table: table_dump <table name>"
        args = line.split()
        self.exactly_n_args(args, 1)
        table_name = args[0]
        table = self.get_res("table", table_name, TABLES)
        entries = self.client.bm_mt_get_entries(0, table_name)

        print "=========="
        print "TABLE ENTRIES"

        for e in entries:
            print "**********"
            self.dump_one_entry(table, e)

        if table.type_ == TableType.indirect or\
           table.type_ == TableType.indirect_ws:
            assert(table.action_prof is not None)
            self._dump_act_prof(table.action_prof)

        # default entry
        default_entry = self.client.bm_mt_get_default_entry(0, table_name)
        print "=========="
        print "Dumping default entry"
        self.dump_action_entry(default_entry)

        print "=========="

    def complete_table_dump(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_table_dump_entry_from_key(self, line):
        "Display some information about a table entry: table_dump_entry_from_key <table name> <match fields> [priority]"
        args = line.split()
        self.at_least_n_args(args, 1)
        table_name = args[0]

        table = self.get_res("table", table_name, TABLES)

        if table.match_type in {MatchType.TERNARY, MatchType.RANGE}:
            try:
                priority = int(args.pop(-1))
            except:
                raise UIn_Error(
                    "Table is ternary, but could not extract a valid priority from args"
                )
        else:
            priority = 0

        match_key = args[1:]
        if len(match_key) != table.num_key_fields():
            raise UIn_Error(
                "Table %s needs %d key fields" % (table_name, table.num_key_fields())
            )
        match_key = parse_match_key(table, match_key)

        entry = self.client.bm_mt_get_entry_from_key(
            0, table_name, match_key, BmAddEntryOptions(priority = priority))
        self.dump_one_entry(table, entry)

    def complete_table_dump_entry_from_key(self, text, line, start_index, end_index):
        return self._complete_tables(text)

    @handle_bad_input
    def do_port_add(self, line):
        "Add a port to the switch (behavior depends on device manager used): port_add <iface_name> <port_num> [pcap_path]"
        args = line.split()
        self.at_least_n_args(args, 2)
        iface_name = args[0]
        try:
            port_num = int(args[1])
        except:
            raise UIn_Error("Bad format for port_num, must be an integer")
        pcap_path = ""
        if len(args) > 2:
            pcap_path = args[2]
        self.client.bm_dev_mgr_add_port(iface_name, port_num, pcap_path)

    @handle_bad_input
    def do_port_remove(self, line):
        "Removes a port from the switch (behavior depends on device manager used): port_remove <port_num>"
        args = line.split()
        self.exactly_n_args(args, 1)
        try:
            port_num = int(args[0])
        except:
            raise UIn_Error("Bad format for port_num, must be an integer")
        self.client.bm_dev_mgr_remove_port(port_num)

    @handle_bad_input
    def do_show_ports(self, line):
        "Shows the ports connected to the switch: show_ports"
        self.exactly_n_args(line.split(), 0)
        ports = self.client.bm_dev_mgr_show_ports()
        print "{:^10}{:^20}{:^10}{}".format(
            "port #", "iface name", "status", "extra info")
        print "=" * 50
        for port_info in ports:
            status = "UP" if port_info.is_up else "DOWN"
            extra_info = "; ".join(
                [k + "=" + v for k, v in port_info.extra.items()])
            print "{:^10}{:^20}{:^10}{}".format(
                port_info.port_num, port_info.iface_name, status, extra_info)

    @handle_bad_input
    def do_switch_info(self, line):
        "Show some basic info about the switch: switch_info"
        self.exactly_n_args(line.split(), 0)
        info = self.client.bm_mgmt_get_info()
        attributes = [t[2] for t in info.thrift_spec[1:]]
        out_attr_w = 5 + max(len(a) for a in attributes)
        for a in attributes:
            print "{:{w}}: {}".format(a, getattr(info, a), w=out_attr_w)

    @handle_bad_input
    def do_reset_state(self, line):
        "Reset all state in the switch (table entries, registers, ...), but P4 config is preserved: reset_state"
        self.exactly_n_args(line.split(), 0)
        self.client.bm_reset_state()

    @handle_bad_input
    def do_write_config_to_file(self, line):
        "Retrieves the JSON config currently used by the switch and dumps it to user-specified file"
        args = line.split()
        self.exactly_n_args(args, 1)
        filename = args[0]
        json_cfg = self.client.bm_get_config()
        with open(filename, 'w') as f:
            f.write(json_cfg)

    @handle_bad_input
    def do_serialize_state(self, line):
        "Serialize the switch state and dumps it to user-specified file"
        args = line.split()
        self.exactly_n_args(args, 1)
        filename = args[0]
        state = self.client.bm_serialize_state()
        with open(filename, 'w') as f:
            f.write(state)

    def set_crc_parameters_common(self, line, crc_width=16):
        conversion_fn = {16: hex_to_i16, 32: hex_to_i32}[crc_width]
        config_type = {16: BmCrc16Config, 32: BmCrc32Config}[crc_width]
        thrift_fn = {16: self.client.bm_set_crc16_custom_parameters,
                     32: self.client.bm_set_crc32_custom_parameters}[crc_width]
        args = line.split()
        self.exactly_n_args(args, 6)
        name = args[0]
        if name not in CUSTOM_CRC_CALCS or CUSTOM_CRC_CALCS[name] != crc_width:
            raise UIn_ResourceError("crc{}_custom".format(crc_width), name)
        config_args = [conversion_fn(a) for a in args[1:4]]
        config_args += [parse_bool(a) for a in args[4:6]]
        crc_config = config_type(*config_args)
        thrift_fn(0, name, crc_config)

    def _complete_crc(self, text, crc_width=16):
        crcs = sorted(
            [c for c, w in CUSTOM_CRC_CALCS.items() if w == crc_width])
        if not text:
            return crcs
        return [c for c in crcs if c.startswith(text)]

    @handle_bad_input
    def do_set_crc16_parameters(self, line):
        "Change the parameters for a custom crc16 hash: set_crc16_parameters <name> <polynomial> <initial remainder> <final xor value> <reflect data?> <reflect remainder?>"
        self.set_crc_parameters_common(line, 16)

    def complete_set_crc16_parameters(self, text, line, start_index, end_index):
        return self._complete_crc(text, 16)

    @handle_bad_input
    def do_set_crc32_parameters(self, line):
        "Change the parameters for a custom crc32 hash: set_crc32_parameters <name> <polynomial> <initial remainder> <final xor value> <reflect data?> <reflect remainder?>"
        self.set_crc_parameters_common(line, 32)

    def complete_set_crc32_parameters(self, text, line, start_index, end_index):
        return self._complete_crc(text, 32)

def load_json_config(standard_client=None, json_path=None):
    load_json_str(utils.get_json_config(standard_client, json_path))

def main():
    args = get_parser().parse_args()

    standard_client, mc_client = thrift_connect(
        args.thrift_ip, args.thrift_port,
        RuntimeAPI.get_thrift_services(args.pre)
    )

    load_json_config(standard_client, args.json)

    RuntimeAPI(args.pre, standard_client, mc_client).cmdloop()

if __name__ == '__main__':
    main()

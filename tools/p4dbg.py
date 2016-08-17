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

import nnpy
import struct
import sys
import json
import argparse
import cmd
from collections import defaultdict
from functools import wraps
import bmpy_utils as utils

try:
    import runtime_CLI
    with_runtime_CLI = True
except:
    with_runtime_CLI = False

parser = argparse.ArgumentParser(description='BM nanomsg debugger')
parser.add_argument('--socket', help='Nanomsg socket to which to subscribe',
                    action="store", required=False)
parser.add_argument('--thrift-port', help='Thrift server port for table updates',
                    type=int, action="store", default=9090)
parser.add_argument('--thrift-ip', help='Thrift IP address for table updates',
                    type=str, action="store", default='localhost')
parser.add_argument('--json', help='JSON description of P4 program [deprecated]',
                    action="store", required=False)

args = parser.parse_args()

class FieldMap:
    def __init__(self):
        self.reset()

    def reset(self):
        self.fields = {}
        self.fields_rev = {}

    def load_names(self, json_cfg):
        self.reset()
        header_types_map = {}
        json_ = json.loads(json_cfg)

        header_types = json_["header_types"]
        for h in header_types:
            header_type = h["name"]
            header_types_map[header_type] = h

        header_instances = json_["headers"]
        for h in header_instances:
            header = h["name"]
            header_type = header_types_map[h["header_type"]]
            for idx, (f_name, f_nbits) in enumerate(header_type["fields"]):
                e = (".".join([header, f_name]), f_nbits)
                self.fields[(h["id"], idx)] = e
                self.fields_rev[e[0]] = (h["id"], idx)

    def get_name(self, header_id, offset):
        return self.fields[(header_id, offset)][0]

    def get_nbits(self, header_id, offset):
        return self.fields[(header_id, offset)][1]

    def get_id_from_name(self, name):
        return self.fields_rev[name]

    def get_all_fields(self):
        return self.fields_rev.keys()


field_map = FieldMap()

def get_name(header_id, offset):
    return field_map.get_name(header_id, offset)

def get_nbits(header_id, offset):
    return field_map.get_nbits(header_id, offset)

def get_field_id_from_name(name):
    header_id, offset = field_map.get_id_from_name(name)
    return (header_id << 32) | offset

def get_name_from_field_id(fid):
    header_id = fid >> 32
    offset = fid & 0xffffffff
    return get_name(header_id, offset)

def get_all_fields():
    return field_map.get_all_fields()

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

class ObjectMap:
    ObjType = enum('ObjType',
                   'PARSER', 'PARSE STATE', 'CONTROL', 'TABLE')

    class _Map:
        def __init__(self):
            self.pname = None
            self.names = {}
            self.names_rev = {}

        def add_one(self, id_, name):
            self.names[id_] = name
            self.names_rev[name] = id_

    def __init__(self):
        self.reset()

    def reset(self):
        self.store = defaultdict(ObjectMap._Map)
        self.ids = {
            1: "parser",
            2: "parse_state",
            3: "pipeline",
            4: "table",
            5: "condition",
            6: "action",
            7: "deparser",
        }
        self.ids_rev = {}
        for k, v in self.ids.items():
            self.ids_rev[v] = k

    def load_names(self, json_cfg):
        json_ = json.loads(json_cfg)
        for type_ in {"parser", "deparser", "action", "pipeline"}:
            _map = self.store[type_]
            _map.pname = type_
            json_list = json_[type_ + "s"]
            for obj in json_list:
                _map.add_one(obj["id"], obj["name"])

        # self.store["pipeline"].pname = "control"
        self.store["pipeline"].pname = "pipeline"

        for pipeline in json_["pipelines"]:
            _map = self.store["table"]
            _map.pname = "table"
            tables = pipeline["tables"]
            for obj in tables:
                _map.add_one(obj["id"], obj["name"])

            _map = self.store["condition"]
            _map.pname = "condition"
            conds = pipeline["conditionals"]
            for obj in conds:
                _map.add_one(obj["id"], obj["name"])

        for parser in json_["parsers"]:
            _map = self.store["parse_state"]
            _map.pname = "parse_state"
            parse_states = parser["parse_states"]
            for obj in parse_states:
                _map.add_one(obj["id"], obj["name"])

    def resolve_ctr(self, ctr):
        type_id = ctr >> 24
        obj_id = ctr & 0xffffff
        type_id = type_id & 0x7f

        assert(type_id in self.ids)
        type_ = self.ids[type_id]
        _map = self.store[type_]

        return _map.pname, _map.names[obj_id]

    def lookup_ctr(self, type_name, obj_name):
        type_id = self.ids_rev[type_name]
        _map = self.store[type_name]
        obj_id = _map.names_rev[obj_name]

        ctr = type_id << 24
        ctr |= obj_id
        return ctr

obj_map = ObjectMap()

def resolve_ctr(ctr):
    return obj_map.resolve_ctr(ctr)

def lookup_ctr(type_name, obj_name):
    return obj_map.lookup_ctr(type_name, obj_name)

MsgType = enum('MsgType',
               'PACKET_IN', 'PACKET_OUT',
               'FIELD_VALUE',
               'CONTINUE',
               'NEXT',
               'GET_VALUE',
               'GET_BACKTRACE', 'BACKTRACE',
               'BREAK_PACKET_IN', 'REMOVE_PACKET_IN',
               'STOP_PACKET_IN', 'RESUME_PACKET_IN',
               'FILTER_NOTIFICATIONS',
               'SET_WATCHPOINT', 'UNSET_WATCHPOINT',
               'STATUS',
               'RESET',
               'KEEP_ALIVE',
               'ATTACH', 'DETACH',)


class Msg(object):
    def __init__(self, **kwargs):
        self.type = None

    def extract(self, msg):
        return 0

    def generate(self):
        return []

    def __str__(self):
        return ""

def make_extract_function(P, fmt):
    def extract(self, msg):
        # investigate why super() not working?
        # offset = super(P, self).extract(msg)
        offset = P.extract(self, msg)
        for name, uf, src in fmt:
            if type(uf) is int:
                width = getattr(self, src)
                f = ">%ds" % (uf * width)
            else:
                assert(src is None)
                assert(type(uf) is str)
                f = uf
                width = struct.calcsize(f)
            v, = struct.unpack_from(f, msg, offset)
            offset += width
            setattr(self, name, v)
            if type(uf) is int:
                if width == 0:
                    v_str = None
                    v_int = None
                else:
                    v_str = ':'.join(x.encode('hex') for x in v)
                    v_int = int(v.encode('hex'), 16)
                setattr(self, name + "_str", v_str)
                setattr(self, name + "_int", v_int)
        return offset

    return extract

def make_str_function(P, fmt):
    def str_func(self):
        s = P.__str__(self)
        for name, uf, _ in fmt:
            s += name + ": "
            if type(uf) is int:
                s += getattr(self, name + "_str")
                s += " (" + str(getattr(self, name + "_int")) + ") "
            else:
                s += str(getattr(self, name))
            s += "\t"
        return s

    return str_func

def make_generate_function(P, fmt):
    def generate(self):
        s = P.generate(self)
        for name, uf, src in fmt:
            v = getattr(self, name)
            if type(uf) is int:
                width = getattr(self, src)
                f = ">%ds" % (uf * width)
            else:
                assert(src is None)
                assert(type(uf) is str)
                f = uf
            # encoding problem if using string
            # nnpy expects unicode, which does not roll
            s += list(struct.pack(f, v))
        return s

    return generate

def make_init_function(P, t, fmt):
    def init_func(self, **kwargs):
        P.__init__(self, **kwargs)
        self.type = t
        if not fmt:
            return
        for k, v in kwargs.items():
            if k not in zip(*fmt)[0]:
                continue
            setattr(self, k, v)

    return init_func

def decorate_msg(P, t, fmt):
    def wrapper(K):
        K.__init__ = make_init_function(P, t, fmt)
        K.extract = make_extract_function(P, fmt)
        K.generate = make_generate_function(P, fmt)
        K.__str__ = make_str_function(P, fmt)
        return K
    return wrapper

@decorate_msg(
    Msg, None,
    [("type", "i", None), ("switch_id", "i", None), ("req_id", "Q", None)],
)
class BasicMsg(Msg):
    pass

@decorate_msg(
    BasicMsg, None,
    [("packet_id", "Q", None), ("copy_id", "Q", None)],
)
class EventMsg(BasicMsg):
    pass

@decorate_msg(
    EventMsg, MsgType.PACKET_IN,
    [("port", "i", None)],
)
class Msg_PacketIn(EventMsg):
    pass

@decorate_msg(
    EventMsg, MsgType.PACKET_OUT,
    [("port", "i", None)],
)
class Msg_PacketOut(EventMsg):
    pass

@decorate_msg(
    EventMsg, MsgType.FIELD_VALUE,
    [("fid", "Q", None), ("nbytes", "i", None), ("bytes", 1, "nbytes")],
)
class Msg_FieldValue(EventMsg):
    pass

def _test():
    m = Msg_FieldValue()
    req = struct.pack("iiQQQQi", MsgType.FIELD_VALUE, 0, 99, 1, 2, 55, 3)
    req += struct.pack(">3s", "\x01\x02\x03")
    m.extract(req)
    s = m.generate()
    assert(s == list(req))

_test()

@decorate_msg(BasicMsg, MsgType.CONTINUE, [])
class Msg_Continue(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.NEXT, [])
class Msg_Next(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.GET_VALUE,
    [("packet_id", "Q", None), ("copy_id", "Q", None), ("fid", "Q", None)],
)
class Msg_GetValue(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.GET_BACKTRACE,
    [("packet_id", "Q", None), ("copy_id", "Q", None)],
)
class Msg_GetBacktrace(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.BACKTRACE,
    [("packet_id", "Q", None), ("copy_id", "Q", None),
     ("nb", "i", None), ("ctrs", 4, "nb")],
)
class Msg_Backtrace(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.BREAK_PACKET_IN, [])
class Msg_BreakPacketIn(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.REMOVE_PACKET_IN, [])
class Msg_RemovePacketIn(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.STOP_PACKET_IN, [])
class Msg_StopPacketIn(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.RESUME_PACKET_IN, [])
class Msg_ResumePacketIn(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.FILTER_NOTIFICATIONS,
    [("nb", "i", None), ("ids", 16, "nb")],
)
class Msg_FilterNotifications(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.SET_WATCHPOINT,
    [("fid", "Q", None)],
)
class Msg_SetWatchpoint(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.UNSET_WATCHPOINT,
    [("fid", "Q", None)],
)
class Msg_UnsetWatchpoint(BasicMsg):
    pass

@decorate_msg(
    BasicMsg, MsgType.STATUS,
    [("status", "i", None), ("aux", "Q", None)],
)
class Msg_Status(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.RESET, [])
class Msg_Reset(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.KEEP_ALIVE, [])
class Msg_KeepAlive(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.ATTACH, [])
class Msg_Attach(BasicMsg):
    pass

@decorate_msg(BasicMsg, MsgType.DETACH, [])
class Msg_Detach(BasicMsg):
    pass

MsgTypeToCLS = {
    MsgType.PACKET_IN : Msg_PacketIn,
    MsgType.PACKET_OUT : Msg_PacketOut,
    MsgType.FIELD_VALUE : Msg_FieldValue,
    MsgType.CONTINUE : Msg_Continue,
    MsgType.NEXT : Msg_Next,
    MsgType.GET_VALUE : Msg_GetValue,
    MsgType.GET_BACKTRACE : Msg_GetBacktrace,
    MsgType.BACKTRACE : Msg_Backtrace,
    MsgType.BREAK_PACKET_IN : Msg_BreakPacketIn,
    MsgType.REMOVE_PACKET_IN : Msg_RemovePacketIn,
    MsgType.STOP_PACKET_IN : Msg_StopPacketIn,
    MsgType.RESUME_PACKET_IN : Msg_ResumePacketIn,
    MsgType.FILTER_NOTIFICATIONS : Msg_FilterNotifications,
    MsgType.SET_WATCHPOINT : Msg_SetWatchpoint,
    MsgType.UNSET_WATCHPOINT : Msg_UnsetWatchpoint,
    MsgType.STATUS : Msg_Status,
    MsgType.RESET : Msg_Reset,
    MsgType.KEEP_ALIVE : Msg_KeepAlive,
    MsgType.ATTACH : Msg_Attach,
    MsgType.DETACH : Msg_Detach,
}

CLSToMsgType = {}
for k, v in MsgTypeToCLS.items():
    CLSToMsgType[v] = k

FIELD_ID_CTR = 0xffffffffffffffff
FIELD_ID_COND = 0xfffffffffffffffe
FIELD_ID_ACTION = 0xfffffffffffffffd

class UIn_Error(Exception):
    def __init__(self, info=""):
        self.info = info

    def __str__(self):
        return self.info

class UIn_Warning(Exception):
    def __init__(self, info=""):
        self.info = info

    def __str__(self):
        return self.info

def handle_bad_input(f):
    @wraps(f)
    def handle(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except UIn_Error as e:
            print "Error:", e
        except UIn_Warning as e:
            print "Error:", e
    return handle

def prompt_yes_no(question, yes_default=True):
    valid = {"yes": True, "y": True, "no": False, "n": False}
    if yes_default:
        prompt = "[Y/n]"
    else:
        prompt = "[y/N]"
    while True:
        print question, prompt,
        choice = raw_input().lower()
        if choice in valid:
            return valid[choice]
        else:
            print "Please respond with 'yes' or 'no' (or 'y' or 'n')."

class DebuggerAPI(cmd.Cmd):
    prompt = 'P4DBG: '
    intro = "Prototype for Debugger UI"

    def __init__(self, addr, standard_client=None):
        cmd.Cmd.__init__(self)
        self.addr = addr
        self.sok = nnpy.Socket(nnpy.AF_SP, nnpy.REQ)
        try:
            self.sok.connect(self.addr)
            self.sok.setsockopt(nnpy.SOL_SOCKET, nnpy.RCVTIMEO, 500)
        except:
            print "Impossible to connect to provided socket (bad format?)"
            sys.exit(1)
        self.req_id = 0
        self.switch_id = 0

        # creates new attributes
        self.reset()

        # creates new attributes
        self.standard_client = standard_client
        self.json_dependent_init()

    def json_dependent_init(self):
        self.json_cfg = utils.get_json_config(
            standard_client=self.standard_client)
        field_map.load_names(self.json_cfg)
        obj_map.load_names(self.json_cfg)

        if not with_runtime_CLI:
            self.with_CLI = False
        else:
            self.with_CLI = True
            self.init_runtime_CLI()

    def reset(self):
        self.wps = set()
        self.bps = set()

        # right now we never remove elements from this
        self.skips = set()
        self.skips_all = set()
        self.current_packet_id = None
        self.current_copy_id = None

        self.get_me_a_prompt = False
        self.attached = False

    def init_runtime_CLI(self):
        runtime_CLI.load_json_str(self.json_cfg)
        self.runtime_CLI = runtime_CLI.RuntimeAPI(
            runtime_CLI.PreType.None, self.standard_client, mc_client=None)

    def attach(self):
        print "Connecting to the switch..."
        self.send_attach()
        self.attached = True
        print "Connection established"

    def send_attach(self):
        req = Msg_Attach(switch_id = 0, req_id = self.get_req_id())
        self.sok.send(req.generate())
        try:
            msg = self.wait_for_msg()
        except AssertionError:
            print "Unable to attach to the switch,",
            print "maybe you need to run p4dbg as root?"
            sys.exit(1)
        self.check_msg_CLS(msg, Msg_Status)

    def do_EOF(self, line):
        print "Detaching from switch"
        self.say_bye()
        return True

    def say_bye(self):
        # This whole function is kind of hacky
        self.sok.setsockopt(nnpy.SOL_SOCKET, nnpy.RCVTIMEO, 500)
        self.sok.setsockopt(nnpy.SOL_SOCKET, nnpy.SNDTIMEO, 500)

        req = Msg_Detach(switch_id = 0, req_id = self.get_req_id())
        status = self.sok.send(req.generate())
        try:
            msg = self.wait_for_msg()
        except:
            return
        self.check_msg_CLS(msg, Msg_Status)
        self.wps = set()

    def send_reset(self):
        req = Msg_Reset(switch_id = 0, req_id = self.get_req_id())
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        self.reset()

    def packet_id_str(self, packet_id, copy_id):
        return ".".join([str(packet_id), str(copy_id)])

    def at_least_n_args(self, args, n):
        if len(args) < n:
            raise UIn_Error("Insufficient number of args")

    def exactly_n_args(self, args, n):
        if len(args) != n:
            raise UIn_Error(
                "Wrong number of args, expected %d but got %d" % (n, len(args))
            )

    def handle_config_change(self):
        self.reset()
        print "We have been notified that the JSON config has changed on the",
        print "switch. The debugger state has therefore been reset."
        print "You can request the new config and keep debugging,",
        print "or we will exit."
        v = prompt_yes_no("Do you want to request the new config?", True)
        if v:
           self.json_dependent_init()
        else:
            sys.exit(0)

    def continue_or_next(self, req, is_next):
        while True:
            self.sok.send(req.generate())
            while True:
                try:
                    msg = self.wait_for_msg()
                    break
                except KeyboardInterrupt:
                    self.get_me_a_prompt = True
                except AssertionError:
                    # happens when there is a connection timeout
                    print "Connection to the switch lost,",
                    print "are you sure it is still running?"
                    return
            if msg.type == MsgType.KEEP_ALIVE:
                if self.get_me_a_prompt == True:
                    self.get_me_a_prompt = False
                    return
            elif msg.type == MsgType.STATUS:
                assert(msg.status == 999)  # config change
                self.handle_config_change()
                return
            else:
                self.check_msg_CLS(msg, EventMsg)
                if msg.packet_id in self.skips_all:
                    continue
                if (msg.packet_id, msg.copy_id) in self.skips:
                    continue
                # a little hacky
                if (not is_next) and (msg.type == MsgType.FIELD_VALUE) and\
                   (msg.fid == FIELD_ID_CTR) and (msg.bytes_int not in self.bps):
                    continue
                break
        self.current_packet_id = msg.packet_id
        self.current_copy_id = msg.copy_id
        print "New event for packet", self.packet_id_str(msg.packet_id, msg.copy_id)
        return msg

    def special_update(self, id_, v):
        if id_ == FIELD_ID_CTR:
            type_name, obj_name = resolve_ctr(v)
            if 0x80 & (v >> 24):
                print "Exiting", type_name, "'%s'" % obj_name
            else:
                print "Entering", type_name, "'%s'" % obj_name
        elif id_ == FIELD_ID_COND:
            print "Condition evaluated to", bool(v)

    @handle_bad_input
    def do_continue(self, line):
        "Starts / resumes packet processing: continue(c)"
        req = Msg_Continue(switch_id = 0, req_id = self.get_req_id())
        msg = self.continue_or_next(req, False)
        if msg is None:
            return
        if msg.type == MsgType.FIELD_VALUE:
            if msg.fid in {FIELD_ID_CTR, FIELD_ID_COND, FIELD_ID_ACTION}:
                self.special_update(msg.fid, msg.bytes_int)
                return
            assert(msg.fid in self.wps)
            print "Watchpoint hit for field", get_name_from_field_id(msg.fid)
            print "New value is", msg.bytes_str, "(%d)" % msg.bytes_int
        elif msg.type == MsgType.PACKET_IN:
            print "Packet in!"
        # elif msg.type == MsgType.PACKET_OUT:
        #     pass
        else:
            assert(0)

    @handle_bad_input
    def do_next(self, line):
        "Go to next field update: next(n)"
        req = Msg_Next(switch_id = 0, req_id = self.get_req_id())
        msg = self.continue_or_next(req, True)
        if msg is None:
            return
        if msg.type == MsgType.FIELD_VALUE:
            if msg.fid in {FIELD_ID_CTR, FIELD_ID_COND, FIELD_ID_ACTION}:
                self.special_update(msg.fid, msg.bytes_int)
                return
            print "Update for field", get_name_from_field_id(msg.fid)
            print "New value is", msg.bytes_str, "(%d)" % msg.bytes_int
        elif msg.type == MsgType.PACKET_IN:
            print "Packet in!"
        # elif msg.type == MsgType.PACKET_OUT:
        #     pass
        else:
            assert(0)

    @handle_bad_input
    def do_reset(self, line):
        "Resets all state, including at the switch"
        self.send_reset()

    def field_id_from_name(self, field_name):
        if field_name == "$cond":
            return FIELD_ID_COND
        elif field_name == "$action":
            return FIELD_ID_ACTION
        else:
            try:
                field_id = get_field_id_from_name(field_name)
                return field_id
            except:
                raise UIn_Error("Unknow field name '%s'" % field_name)

    @handle_bad_input
    def do_set_wp(self, line):
        "Sets a watchpoint on a field: set_wp(w) <field_name>"
        args = line.split()
        self.exactly_n_args(args, 1)

        field_id = self.field_id_from_name(args[0])

        req = Msg_SetWatchpoint(
            switch_id = 0, req_id = self.get_req_id(), fid = field_id
        )
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)
        self.wps.add(field_id)

    def complete_set_wp(self, text, line, start_index, end_index):
        fields = sorted(get_all_fields())
        if not text:
            return fields
        return [f for f in fields if f.startswith(text)]

    @handle_bad_input
    def do_unset_wp(self, line):
        "Unsets a watchpoint on a field: unset_wp <field_name>"
        args = line.split()
        self.exactly_n_args(args, 1)

        field_id = self.field_id_from_name(args[0])

        if field_id not in self.wps:
            raise UIn_Warning("No watchpoint previously set for this field")

        req = Msg_UnsetWatchpoint(
            switch_id = 0, req_id = self.get_req_id(), fid = field_id
        )
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)
        self.wps.remove(field_id)

    def complete_unset_wp(self, text, line, start_index, end_index):
        fields = sorted([get_name_from_field_id(f) for f in self.wps])
        if not text:
            return fields
        return [f for f in fields if f.startswith(text)]

    @handle_bad_input
    def do_show_wps(self, line):
        "Shows all the watchpoints set: show_wps"
        for f_name in map(get_name_from_field_id, self.wps):
            print f_name

    @handle_bad_input
    def do_break(self, line):
        "Sets a break point for a parser, parse_state, pipeline, table, condition, action or deparser: break(b) <obj type (e.g. parser)> <obj name (e.g. start)>"
        args = line.split()
        self.exactly_n_args(args, 2)

        try:
            ctr = lookup_ctr(args[0], args[1])
        except:
            raise UIn_Error("Invalid object for breakpoint")

        if not self.bps:
            req = Msg_SetWatchpoint(
                switch_id = 0, req_id = self.get_req_id(), fid = FIELD_ID_CTR
            )
            self.sok.send(req.generate())
            msg = self.wait_for_msg()
            self.check_msg_CLS(msg, Msg_Status)
            assert(msg.status == 0)
        self.bps.add(ctr)

    def complete_break(self, text, line, start_index, end_index):
        args = line.split()
        args_cnt = len(args)
        obj_types = obj_map.ids.values()
        fields = sorted(get_all_fields())
        if (args_cnt == 1 and not text) or\
           (args_cnt == 2 and text):
            return [t for t in obj_types if t.startswith(text)]
        if (args_cnt == 2 and not text) or\
           (args_cnt == 3 and text):
            obj_type = args[1]
            obj_names = obj_map.store[obj_type].names.values()
            return [n for n in obj_names if n.startswith(text)]
        return []

    @handle_bad_input
    def do_delete(self, line):
        "Unsets a break point, previously set with the 'break' command: delete(d) <obj_type> <obj_name>"
        args = line.split()
        self.exactly_n_args(args, 2)

        try:
            ctr = lookup_ctr(args[0], args[1])
        except:
            raise UIn_Error("Invalid object for breakpoint")

        if ctr not in self.bps:
            raise UIn_Warning("No breakpoint previously set for this object")

        self.bps.remove(ctr)

    def complete_delete(self, text, line, start_index, end_index):
        args = line.split()
        args_cnt = len(args)
        if not self.bps:
            return []
        objs = map(resolve_ctr, self.bps)
        if (args_cnt == 1 and not text) or\
           (args_cnt == 2 and text):
            return [t for t in zip(*objs)[0] if t.startswith(text)]
        if (args_cnt == 2 and not text) or\
           (args_cnt == 3 and text):
            return [n for t, n in objs if t == args[1] and n.startswith(text)]
        return []

    @handle_bad_input
    def do_show_bps(self, line):
        "Shows all the breakpoints set: show_bps"
        for t, n in map(resolve_ctr, self.bps):
            print t, "'%s'" % n

    def get_req_id(self):
        req_id = self.req_id
        self.req_id += 1
        return req_id

    def current_packet_id_str(self):
        return ".".join([str(self.current_packet_id),
                         str(self.current_copy_id)])

    def packet_id_from_str(self, id_):
        try:
            packet_id, copy_id = id_.split(".")
            packet_id, copy_id = int(packet_id), int(copy_id)
            return packet_id, copy_id
        except:
            raise UIn_Error("Bad packet id format '%s', should be of the form <1>.<2>" % id_)

    @handle_bad_input
    def do_print(self, line):
        "Prints the value of a field for a packet: print(p) <packet_id> <field_name>"
        args = line.split()
        self.at_least_n_args(args, 1)

        if len(args) == 1:
            if self.current_packet_id is None:
                raise UIn_Error("Cannot use this command because no packets have been observed yet")
            print "Packet id not specified, assuming current packet (%s)" \
                % self.current_packet_id_str()
            packet_id = self.current_packet_id
            copy_id = self.current_copy_id
            field_name = args[0]
        else:
            packet_id, copy_id = self.packet_id_from_str(args[0])
            field_name = args[1]

        field_id = self.field_id_from_name(field_name)

        req = Msg_GetValue(
            switch_id = 0, req_id = self.get_req_id(),
            packet_id = packet_id, copy_id = copy_id, fid = field_id
        )
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        if msg.type == MsgType.STATUS:
            if msg.status == 1:
                print "Unknown packet id. Maybe it left the switch already"
            elif msg.status == 2:
                print "We have not received a value for this field,"\
                    " maybe it is not valid (yet), or the value is 0"
                pass
            else:
                assert(0)
            return
        self.check_msg_CLS(msg, Msg_FieldValue)
        print msg.bytes_str, "(%d)" % msg.bytes_int

    def complete_print(self, text, line, start_index, end_index):
        args = line.split()
        args_cnt = len(args)
        fields = sorted(get_all_fields())
        return [f for f in fields if f.startswith(text)]

    @handle_bad_input
    def do_backtrace(self, line):
        "Show a backtrace of contexts entered by packet: backtrace(bt) <packet_id>"
        args = line.split()

        if len(args) == 0:
            if self.current_packet_id is None:
                raise UIn_Error("Cannot use this command because no packets have been observed yet")
            print "Packet id not specified, assuming current packet (%s)" \
                % self.current_packet_id_str()
            packet_id = self.current_packet_id
            copy_id = self.current_copy_id
        else:
            packet_id, copy_id = self.packet_id_from_str(args[0])

        req = Msg_GetBacktrace(
            switch_id = 0, req_id = self.get_req_id(),
            packet_id = packet_id, copy_id = copy_id
        )
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        if msg.type == MsgType.STATUS:
            if msg.status == 1:
                print "Unknown packet id. Maybe it left the switch already"
            else:
                assert(0)
            return
        self.check_msg_CLS(msg, Msg_Backtrace)
        ctrs = struct.unpack("%di" % msg.nb, msg.ctrs)
        bt = []
        for ctr in ctrs:
            type_name, obj_name = resolve_ctr(ctr)
            bt += ["%s '%s'" % (type_name, obj_name)]
        print " -> ".join(bt)

    @handle_bad_input
    def do_break_packet_in(self, line):
        "Breaks everytime a new packet enters the switch"
        req = Msg_BreakPacketIn(switch_id = 0, req_id = self.get_req_id())
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)
        self.break_packet_in = True

    @handle_bad_input
    def do_remove_packet_in(self, line):
        "Remove breakpoint set by break_packet_in"
        if not self.break_packet_in:
            print "Packet in breakpoint was not previously set"
            return
        req = Msg_RemovePacketIn(switch_id = 0, req_id = self.get_req_id())
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)
        self.break_packet_in = False

    @handle_bad_input
    def do_stop_packet_in(self, line):
        "Stop accepting packets into the switch"
        req = Msg_StopPacketIn(switch_id = 0, req_id = self.get_req_id())
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)
        self.stop_packet_in = True

    @handle_bad_input
    def do_resume_packet_in(self, line):
        "Start accepting packets into the switch again (undoes stop_packet_in)"
        if not self.stop_packet_in:
            print "stop_packet_in was not being enforced"
            return
        req = Msg_ResumePacketIn(switch_id = 0, req_id = self.get_req_id())
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)
        self.stop_packet_in = False

    @handle_bad_input
    def do_filter_notifications(self, line):
        "Filter notifications to some packet_ids: filter_notifications <packet ids>*"
        args = line.split()
        pids = []
        for pid in args:
            if "." in pid:
                packet_id, copy_id = self.packet_id_from_str(args[0])
            else:
                try:
                    packet_id = int(pid)
                    copy_id = 0xffffffffffffffff
                except:
                    raise UIn_Error("Bad format for packet id")
            pids.append( (packet_id, copy_id) )

        s = ""
        for packet_id, copy_id in pids:
            s += struct.pack("QQ", packet_id, copy_id)

        req = Msg_FilterNotifications(
            switch_id = 0, req_id = self.get_req_id(),
            nb = len(pids), ids = s
        )
        self.sok.send(req.generate())
        msg = self.wait_for_msg()
        self.check_msg_CLS(msg, Msg_Status)
        assert(msg.status == 0)

    @handle_bad_input
    def do_skip(self, line):
        "Skip all future notifications for this packet"
        if self.current_packet_id is None:
            return
        assert(self.current_copy_id is not None)
        self.skips.add( (self.current_packet_id, self.current_copy_id) )

    @handle_bad_input
    def do_skip_all(self, line):
        "Skip all future notifications for this packet and its descendants"
        if self.current_packet_id is None:
            return
        assert(self.current_copy_id is not None)
        self.skips_all.add(self.current_packet_id)

    def do_CLI(self, line):
        "Connects to the bmv2 using the CLI, e.g. CLI table_dump <table_name>"
        if not with_runtime_CLI or not self.standard_client:
            return UIn_Error("Runtime CLI not available")
        self.runtime_CLI.onecmd(line)

    def complete_CLI(self, text, line, start_index, end_index):
        args_ = line.split()
        args_cnt = len(args_)
        if (args_cnt == 1 and not text) or\
           (args_cnt == 2 and text):
            return self.runtime_CLI.completenames(" ".join(args_[1:]))
        else:
            complete = "complete_" + args_[1]
            method = getattr(self.runtime_CLI, complete)
            return method(text, " ".join(args_[1:]), start_index, end_index)

    def wait_for_msg(self):
        data = self.sok.recv()
        msg = BasicMsg()
        msg.extract(data)
        msg_type = msg.type
        msg = MsgTypeToCLS[msg_type]()
        msg.extract(data)
        # print "Message received"
        # print msg
        return msg

    def check_msg_CLS(self, msg, CLS):
        assert(isinstance(msg, CLS))

# Adding command shortcuts
shortcuts = {
    "continue" : "c",
    "next" : "n",
    "print" : "p",
    "set_wp" : "w",
    "backtrace" : "bt",
    "break" : "b",
    "delete" : "d",
}
for long_name, short_name in shortcuts.items():
    f_do = getattr(DebuggerAPI, "do_" + long_name)
    setattr(DebuggerAPI, "do_" + short_name, f_do)
    if hasattr(DebuggerAPI, "complete_" + long_name):
        f_complete = getattr(DebuggerAPI, "complete_" + long_name)
        setattr(DebuggerAPI, "complete_" + short_name, f_complete)

def main():
    deprecated_args = ["json"]
    for a in deprecated_args:
        if getattr(args, a) is not None:
            print "Command line option '--{}' is deprecated".format(a),
            print "and will be ignored"

    client = utils.thrift_connect_standard(args.thrift_ip, args.thrift_port)
    info = client.bm_mgmt_get_info()
    socket_addr = info.debugger_socket
    if socket_addr is None:
        print "The debugger is not enabled on the switch"
        sys.exit(1)
    if args.socket is not None:
        socket_addr = args.socket
    else:
        print "'--socket' not provided, using", socket_addr,
        print "(obtained from switch)"

    c = DebuggerAPI(socket_addr, standard_client=client)
    try:
        c.attach()
        c.cmdloop()
    except KeyboardInterrupt:
        if c.attached:
            c.say_bye()
    except Exception as e:
        print "Unknow error, detaching and exiting"
        print e
        if c.attached:
            c.say_bye()

if __name__ == "__main__":
    main()

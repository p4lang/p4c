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

import nnpy
import struct
import sys
import json
import argparse

parser = argparse.ArgumentParser(description='BM nanomsg event logger client')
parser.add_argument('--socket', help='IPC socket to which to subscribe',
                    action="store", default="ipc:///tmp/bm-0-log.ipc")

parser.add_argument('--json', help='JSON description of P4 program',
                    action="store")

args = parser.parse_args()

class NameMap:
    def __init__(self):
        self.names = {}

    def load_names(self, json_src):
        with open(json_src, 'r') as f:
            json_ = json.load(f)
            
            for type_ in {"header_type", "header", "parser",
                          "deparser", "action", "pipeline", "checksum"}:
                json_list = json_[type_ + "s"]
                for obj in json_list:
                    self.names[(type_, obj["id"])] = obj["name"]

            for pipeline in json_["pipelines"]:
                tables = pipeline["tables"]
                for obj in tables:
                    self.names[("table", obj["id"])] = obj["name"]

                conds = pipeline["conditionals"]
                for obj in conds:
                    self.names[("condition", obj["id"])] = obj["name"]

    def get_name(self, type_, id_):
        return self.names.get( (type_, id_), None )

name_map = NameMap()

def name_lookup(type_, id_):
    return name_map.get_name(type_, id_)

class MSG_TYPES:
    (PACKET_IN, PACKET_OUT,
     PARSER_START, PARSER_DONE, PARSER_EXTRACT,
     DEPARSER_START, DEPARSER_DONE, DEPARSER_EMIT,
     CHECKSUM_UPDATE,
     PIPELINE_START, PIPELINE_DONE,
     CONDITION_EVAL, TABLE_HIT, TABLE_MISS,
     ACTION_EXECUTE) = range(15)

    @staticmethod
    def get_msg_class(type_):
        classes = {
            MSG_TYPES.PACKET_IN: PacketIn,
            MSG_TYPES.PACKET_OUT: PacketOut,
            MSG_TYPES.PARSER_START: ParserStart,
            MSG_TYPES.PARSER_DONE: ParserDone,
            MSG_TYPES.PARSER_EXTRACT: ParserExtract,
            MSG_TYPES.DEPARSER_START: DeparserStart,
            MSG_TYPES.DEPARSER_DONE: DeparserDone,
            MSG_TYPES.DEPARSER_EMIT: DeparserEmit,
            MSG_TYPES.CHECKSUM_UPDATE: ChecksumUpdate,
            MSG_TYPES.PIPELINE_START: PipelineStart,
            MSG_TYPES.PIPELINE_DONE: PipelineDone,
            MSG_TYPES.CONDITION_EVAL: ConditionEval,
            MSG_TYPES.TABLE_HIT: TableHit,
            MSG_TYPES.TABLE_MISS: TableMiss,
            MSG_TYPES.ACTION_EXECUTE: ActionExecute,
        }
        return classes[type_]

    @staticmethod
    def get_str(type_):
        strs = {
            MSG_TYPES.PACKET_IN: "PACKET_IN",
            MSG_TYPES.PACKET_OUT: "PACKET_OUT",
            MSG_TYPES.PARSER_START: "PARSER_START",
            MSG_TYPES.PARSER_DONE: "PARSER_DONE",
            MSG_TYPES.PARSER_EXTRACT: "PARSER_EXTRACT",
            MSG_TYPES.DEPARSER_START: "DEPARSER_START",
            MSG_TYPES.DEPARSER_DONE: "DEPARSER_DONE",
            MSG_TYPES.DEPARSER_EMIT: "DEPARSER_EMIT",
            MSG_TYPES.CHECKSUM_UPDATE: "CHECKSUM_UPDATE",
            MSG_TYPES.PIPELINE_START: "PIPELINE_START",
            MSG_TYPES.PIPELINE_DONE: "PIPELINE_DONE",
            MSG_TYPES.CONDITION_EVAL: "CONDITION_EVAL",
            MSG_TYPES.TABLE_HIT: "TABLE_HIT",
            MSG_TYPES.TABLE_MISS: "TABLE_MISS",
            MSG_TYPES.ACTION_EXECUTE: "ACTION_EXECUTE",
        }
        return strs[type_]

class Msg(object):
    def __init__(self, msg):
        self.msg = msg

    def extract_hdr(self):
        struct_ = struct.Struct("iiQQQ")
        (_, self.switch_id,
         self.sig, self.id_, self.copy_id) = struct_.unpack_from(self.msg)
        return struct_.size

    def extract(self):
        bytes_extracted = self.extract_hdr()
        msg_remainder = self.msg[bytes_extracted:]
        return self.struct_.unpack(msg_remainder)

    def __str__(self):
        return "type: %s, switch_id: %d, sig: %d, id: %d, copy_id: %d" %\
            (self.type_str, self.switch_id,
             self.sig, self.id_, self.copy_id)

class PacketIn(Msg):
    def __init__(self, msg):
        super(PacketIn, self).__init__(msg)
        self.type_ = MSG_TYPES.PACKET_IN
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.port_in, = super(PacketIn, self).extract()

    def __str__(self):
        return super(PacketIn, self).__str__() +\
            ", port_in: %d" % self.port_in

class PacketOut(Msg):
    def __init__(self, msg):
        super(PacketOut, self).__init__(msg)
        self.type_ = MSG_TYPES.PACKET_OUT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.port_out, = super(PacketOut, self).extract()

    def __str__(self):
        return super(PacketOut, self).__str__() +\
            ", port_out: %d" % self.port_out

class ParserStart(Msg):
    def __init__(self, msg):
        super(ParserStart, self).__init__(msg)
        self.type_ = MSG_TYPES.PARSER_START
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.parser_id, = super(ParserStart, self).extract()

    def __str__(self):
        s = super(ParserStart, self).__str__()
        s += ", parser_id: " +  str(self.parser_id)
        name = name_lookup("parser", self.parser_id)
        if name: s += " (" + name + ")"
        return s

class ParserDone(Msg):
    def __init__(self, msg):
        super(ParserDone, self).__init__(msg)
        self.type_ = MSG_TYPES.PARSER_DONE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.parser_id, = super(ParserDone, self).extract()

    def __str__(self):
        s = super(ParserDone, self).__str__()
        s += ", parser_id: " +  str(self.parser_id)
        name = name_lookup("parser", self.parser_id)
        if name: s += " (" + name + ")"
        return s

class ParserExtract(Msg):
    def __init__(self, msg):
        super(ParserExtract, self).__init__(msg)
        self.type_ = MSG_TYPES.PARSER_EXTRACT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.header_id, = super(ParserExtract, self).extract()

    def __str__(self):
        s = super(ParserExtract, self).__str__()
        s += ", header_id: " +  str(self.header_id)
        name = name_lookup("header", self.header_id)
        if name: s += " (" + name + ")"
        return s

class DeparserStart(Msg):
    def __init__(self, msg):
        super(DeparserStart, self).__init__(msg)
        self.type_ = MSG_TYPES.DEPARSER_START
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.deparser_id, = super(DeparserStart, self).extract()

    def __str__(self):
        s = super(DeparserStart, self).__str__()
        s += ", deparser_id: " +  str(self.deparser_id)
        name = name_lookup("deparser", self.deparser_id)
        if name: s += " (" + name + ")"
        return s

class DeparserDone(Msg):
    def __init__(self, msg):
        super(DeparserDone, self).__init__(msg)
        self.type_ = MSG_TYPES.DEPARSER_DONE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.deparser_id, = super(DeparserDone, self).extract()

    def __str__(self):
        s = super(DeparserDone, self).__str__()
        s += ", deparser_id: " +  str(self.deparser_id)
        name = name_lookup("deparser", self.deparser_id)
        if name: s += " (" + name + ")"
        return s

class DeparserEmit(Msg):
    def __init__(self, msg):
        super(DeparserEmit, self).__init__(msg)
        self.type_ = MSG_TYPES.DEPARSER_EMIT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
         self.header_id, = super(DeparserEmit, self).extract()

    def __str__(self):
        s = super(DeparserEmit, self).__str__()
        s += ", header_id: " +  str(self.header_id)
        name = name_lookup("header", self.header_id)
        if name: s += " (" + name + ")"
        return s

class ChecksumUpdate(Msg):
    def __init__(self, msg):
        super(ChecksumUpdate, self).__init__(msg)
        self.type_ = MSG_TYPES.CHECKSUM_UPDATE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
         self.cksum_id, = super(ChecksumUpdate, self).extract()

    def __str__(self):
        s = super(ChecksumUpdate, self).__str__()
        s += ", cksum_id: " +  str(self.cksum_id)
        name = name_lookup("checksum", self.cksum_id)
        if name: s += " (" + name + ")"
        return s

class PipelineStart(Msg):
    def __init__(self, msg):
        super(PipelineStart, self).__init__(msg)
        self.type_ = MSG_TYPES.PIPELINE_START
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.pipeline_id, = super(PipelineStart, self).extract()

    def __str__(self):
        s = super(PipelineStart, self).__str__()
        s += ", pipeline_id: " +  str(self.pipeline_id)
        name = name_lookup("pipeline", self.pipeline_id)
        if name: s += " (" + name + ")"
        return s

class PipelineDone(Msg):
    def __init__(self, msg):
        super(PipelineDone, self).__init__(msg)
        self.type_ = MSG_TYPES.PIPELINE_DONE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.pipeline_id, = super(PipelineDone, self).extract()

    def __str__(self):
        s = super(PipelineDone, self).__str__()
        s += ", pipeline_id: " +  str(self.pipeline_id)
        name = name_lookup("pipeline", self.pipeline_id)
        if name: s += " (" + name + ")"
        return s

class ConditionEval(Msg):
    def __init__(self, msg):
        super(ConditionEval, self).__init__(msg)
        self.type_ = MSG_TYPES.CONDITION_EVAL
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("ii")
        
    def extract(self):
        self.condition_id, self.result = super(ConditionEval, self).extract()
        self.result = True if self.result != 0 else False

    def __str__(self):
        s = super(ConditionEval, self).__str__()
        s += ", condition_id: " +  str(self.condition_id)
        name = name_lookup("condition", self.condition_id)
        if name: s += " (" + name + ")"
        s += ", result: " + str(self.result)
        return s


class TableHit(Msg):
    def __init__(self, msg):
        super(TableHit, self).__init__(msg)
        self.type_ = MSG_TYPES.TABLE_HIT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("ii")
        
    def extract(self):
        self.table_id, self.entry_hdl = super(TableHit, self).extract()

    def __str__(self):
        s = super(TableHit, self).__str__()
        s += ", table_id: " +  str(self.table_id)
        name = name_lookup("table", self.table_id)
        if name: s += " (" + name + ")"
        s += ", entry_hdl: " +  str(self.entry_hdl)
        return s

class TableMiss(Msg):
    def __init__(self, msg):
        super(TableMiss, self).__init__(msg)
        self.type_ = MSG_TYPES.TABLE_MISS
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.table_id, = super(TableMiss, self).extract()

    def __str__(self):
        s = super(TableMiss, self).__str__()
        s += ", table_id: " +  str(self.table_id)
        name = name_lookup("table", self.table_id)
        if name: s += " (" + name + ")"
        return s

class ActionExecute(Msg):
    def __init__(self, msg):
        super(ActionExecute, self).__init__(msg)
        self.type_ = MSG_TYPES.ACTION_EXECUTE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.action_id, = super(ActionExecute, self).extract()

    def __str__(self):
        s = super(ActionExecute, self).__str__()
        s += ", action_id: " +  str(self.action_id)
        name = name_lookup("action", self.action_id)
        if name: s += " (" + name + ")"
        return s

def recv_msgs():
    def get_msg_type(msg):
        type_, = struct.unpack('i', msg[:4])
        return type_

    sub = nnpy.Socket(nnpy.AF_SP, nnpy.SUB)
    sub.connect(args.socket)
    sub.setsockopt(nnpy.SUB, nnpy.SUB_SUBSCRIBE, '')
    
    while True:
        msg = sub.recv()
        msg_type = get_msg_type(msg)

        try:
            p = MSG_TYPES.get_msg_class(msg_type)(msg)
        except:
            print "Unknown msg type", msg_type
            continue
        p.extract()
        print p

def main():
    json_src = args.json

    if json_src:
        name_map.load_names(json_src)

    recv_msgs()

if __name__ == "__main__":
    main()

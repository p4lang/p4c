#!/usr/bin/env python2
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

""" Converts the commands in an stf file which populate tables into a C
    program that manipulates ebpf tables. """


import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../../tools')
from testutils import *
from stf.stf_parser import STFParser


class eBPFCommand(object):
    """ Defines a match-action command for eBPF programs"""

    def __init__(self, a_type, table, action, priority="", match=[], extra=""):
        self.a_type = a_type            # dir in which all files are stored
        self.table = table          # contains meta information
        self.action = action          # contains meta information
        self.priority = priority    # template to generate a filter
        self.match = match          # contains standard and error output
        self.extra = extra          # could also be "pcapng"


def _generate_control_actions(cmds):
    """ Generates the actual control plane commands.
    This function inserts C code for all the "add" commands that have
    been parsed. """
    generated = ""
    for index, cmd in enumerate(cmds):
        key_name = "key_%s%d" % (cmd.table, index)
        value_name = "value_%s%d" % (cmd.table, index)
        if cmd.a_type == "setdefault":
            tbl_name = cmd.table + "_defaultAction"
            generated += "u32 %s = 0;\n\t" % (key_name)
        else:
            generated += "struct %s_key %s = {};\n\t" % (cmd.table, key_name)
            tbl_name = cmd.table
            for key_num, key_field in enumerate(cmd.match):
                field = key_field[0].split('.')[1]
                generated += ("%s.%s = %s;\n\t"
                              % (key_name, field, key_field[1]))
        generated += ("tableFileDescriptor = "
                      "BPF_OBJ_GET(MAP_PATH \"/%s\");\n\t" %
                      tbl_name)
        generated += ("if (tableFileDescriptor < 0) {"
                      "fprintf(stderr, \"map %s not loaded\");"
                      " exit(1); }\n\t" % tbl_name)
        generated += ("struct %s_value %s = {\n\t\t" % (
            cmd.table, value_name))
        generated += ".action = %s,\n\t\t" % (cmd.action[0])
        generated += ".u = {.%s = {" % cmd.action[0]
        for val_num, val_field in enumerate(cmd.action[1]):
            generated += "%s," % val_field[1]
        generated += "}},\n\t"
        generated += "};\n\t"
        generated += ("ok = BPF_USER_MAP_UPDATE_ELEM"
                      "(tableFileDescriptor, &%s, &%s, BPF_ANY);\n\t"
                      % (key_name, value_name))
        generated += ("if (ok != 0) { perror(\"Could not write in %s\");"
                      "exit(1); }\n" % tbl_name)
    return generated


def create_table_file(actions, tmpdir, file_name):
    """ Create the control plane file.
    The control commands are provided by the stf parser.
    This generated file is required by ebpf_runtime.c to initialize
    the control plane. """
    err = ""
    try:
        with open(tmpdir + "/" + file_name, "w+") as control_file:
            control_file.write("#include \"test.h\"\n\n")
            control_file.write("static inline void setup_control_plane() {")
            control_file.write("\n\t")
            control_file.write("int ok;\n\t")
            control_file.write("int tableFileDescriptor;\n\t")
            generated_cmds = _generate_control_actions(actions)
            control_file.write(generated_cmds)
            control_file.write("}\n")
    except OSError as e:
        err = e
        return FAILURE, err
    return SUCCESS, err


def parse_stf_file(raw_stf):
    """ Uses the .stf parsing tool to acquire a pre-formatted list.
        Processing entries according to their specified cmd. """
    parser = STFParser()
    stf_str = raw_stf.read()
    stf_map, errs = parser.parse(stf_str)
    input_pkts = {}
    cmds = []
    expected = {}
    for stf_entry in stf_map:
        if stf_entry[0] == "packet":
            input_pkts.setdefault(stf_entry[1], []).append(
                hex_to_byte(stf_entry[2]))
        elif stf_entry[0] == "expect":
            interface = int(stf_entry[1])
            pkt_data = stf_entry[2]
            expected.setdefault(interface, {})
            if pkt_data != '':
                expected[interface]["any"] = False
                expected[interface].setdefault(
                    "pkts", []).append(pkt_data)
            else:
                expected[interface]["any"] = True
        elif stf_entry[0] == "add":
            cmd = eBPFCommand(
                a_type=stf_entry[0], table=stf_entry[1],
                priority=stf_entry[2], match=stf_entry[3],
                action=stf_entry[4], extra=stf_entry[5])
            cmds.append(cmd)
        elif stf_entry[0] == "setdefault":
            cmd = eBPFCommand(
                a_type=stf_entry[0], table=stf_entry[1], action=stf_entry[2])
            cmds.append(cmd)
    return input_pkts, cmds, expected

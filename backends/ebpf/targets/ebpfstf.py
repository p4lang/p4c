#!/usr/bin/env python3

# Copyright 2018 VMware, Inc.
# SPDX-FileCopyrightText: 2018 VMware, Inc.
#
# SPDX-License-Identifier: Apache-2.0
""" Converts the commands in an stf file which populate tables into a C
    program that manipulates ebpf tables. """

import sys
from pathlib import Path

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../../tools")))
import testutils
from stf.stf_parser import STFParser


class eBPFCommand(object):
    """Defines a match-action command for eBPF programs."""

    def __init__(self, a_type, table, action, priority="", match=[], extra=""):
        # Dir in which all files are stored.
        self.a_type = a_type
        # Contains meta information.
        self.table = table
        # Contains meta information.
        self.action = action
        # Template to generate a filter.
        self.priority = priority
        # Contains standard and error output.
        self.match = match
        # Could also be "pcapng".
        self.extra = extra


def _generate_control_actions(cmds):
    """Generates the actual control plane commands.
    This function inserts C code for all the "add" commands that have
    been parsed."""
    generated = ""
    for index, cmd in enumerate(cmds):
        table_name = cmd.table.replace(".", "_")
        key_name = "key_%s%d" % (table_name, index)
        value_name = "value_%s%d" % (table_name, index)
        if cmd.a_type == "setdefault":
            tbl_name = table_name + "_defaultAction"
            generated += "u32 %s = 0;\n\t" % (key_name)
        else:
            generated += "struct %s_key %s = {};\n\t" % (table_name, key_name)
            tbl_name = table_name
            for key_num, key_field in enumerate(cmd.match):
                field = "field%s" % key_num
                key_field_val = key_field[1]
                # Support for LPM key
                if isinstance(key_field_val, tuple):
                    generated += (
                        "%s.prefixlen = (offsetof(struct %s_key, %s) - 4) * 8 + %s;\n\t"
                        % (key_name, table_name, field, key_field_val[1])
                    )
                    key_field_val = key_field_val[0]
                generated += "%s.%s = %s;\n\t" % (key_name, field, key_field_val)
        generated += "tableFileDescriptor = BPF_OBJ_GET(MAP_PATH \"/%s\");\n\t" % tbl_name
        generated += (
            "if (tableFileDescriptor < 0) {fprintf(stderr, \"map %s not loaded\"); exit(1); }\n\t"
            % tbl_name
        )
        generated += "struct %s_value %s = {\n\t\t" % (table_name, value_name)
        action_name = cmd.action[0]
        action_c_name = action_name.replace(".", "_")
        if action_name in ("_NoAction", "NoAction"):
            generated += ".action = 0,\n\t\t"
            action_c_name = "_NoAction"
        else:
            action_full_name = "{}_ACT_{}".format(table_name.upper(), action_c_name.upper())
            generated += ".action = %s,\n\t\t" % action_full_name
        generated += ".u = {.%s = {" % action_c_name
        for val_num, val_field in enumerate(cmd.action[1]):
            generated += "%s," % val_field[1]
        generated += "}},\n\t"
        generated += "};\n\t"
        generated += (
            "ok = BPF_USER_MAP_UPDATE_ELEM(tableFileDescriptor, &%s, &%s, BPF_ANY);\n\t"
            % (key_name, value_name)
        )
        generated += 'if (ok != 0) { perror("Could not write in %s");exit(1); }\n' % tbl_name
    return generated


def create_table_file(actions, tmpdir, file_name):
    """Create the control plane file.
    The control commands are provided by the stf parser.
    This generated file is required by ebpf_runtime.c to initialize
    the control plane."""
    err = ""
    try:
        with open(tmpdir + "/" + file_name, "w+") as control_file:
            control_file.write('#include "test.h"\n')
            control_file.write("#include <stddef.h>\n\n")
            control_file.write("static inline void setup_control_plane() {")
            control_file.write("\n\t")
            control_file.write("int ok;\n\t")
            control_file.write("int tableFileDescriptor;\n\t")
            generated_cmds = _generate_control_actions(actions)
            control_file.write(generated_cmds)
            control_file.write("}\n")
    except OSError as e:
        err = e
        return testutils.FAILURE, err
    return testutils.SUCCESS, err


def parse_stf_file(raw_stf):
    """Uses the .stf parsing tool to acquire a pre-formatted list.
    Processing entries according to their specified cmd."""
    parser = STFParser()
    stf_str = raw_stf.read()
    stf_map, errs = parser.parse(stf_str)
    input_pkts = {}
    cmds = []
    expected = {}
    for stf_entry in stf_map:
        if stf_entry[0] == "packet":
            interface = int(stf_entry[1])
            data = stf_entry[2]
            input_pkts.setdefault(interface, []).append(bytes.fromhex("".join(data.split())))
        elif stf_entry[0] == "expect":
            interface = int(stf_entry[1])
            pkt_data = stf_entry[2]
            expected.setdefault(interface, {})
            if pkt_data != "":
                expected[interface]["any"] = False
                expected[interface].setdefault("pkts", []).append(pkt_data)
            else:
                expected[interface]["any"] = True
        elif stf_entry[0] == "add":
            cmd = eBPFCommand(
                a_type=stf_entry[0],
                table=stf_entry[1],
                priority=stf_entry[2],
                match=stf_entry[3],
                action=stf_entry[4],
                extra=stf_entry[5],
            )
            cmds.append(cmd)
        elif stf_entry[0] == "setdefault":
            cmd = eBPFCommand(a_type=stf_entry[0], table=stf_entry[1], action=stf_entry[2])
            cmds.append(cmd)
    return input_pkts, cmds, expected

#!/usr/bin/python

import argparse
import cmd
import os
import sys
import struct
import json
import re

from tenjin.util import *

tenjin_prefix = "//::" # Use // in prefix for C syntax processing

parser = argparse.ArgumentParser(description='PD library C code generator')
parser.add_argument('--json', help='JSON description of P4 program',
                    type=str, action="store", required=True)
parser.add_argument('--p4-prefix', help='P4 name use for API function prefix',
                    type=str, action="store", required=True)

args = parser.parse_args()

_THIS_DIR = os.path.dirname(os.path.realpath(__file__))

templates_dir = os.path.join(_THIS_DIR, "templates")

TABLES = {}
ACTIONS = {}

class MatchType:
    EXACT = 0
    LPM = 1
    TERNARY = 2
    VALID = 3

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
                table.with_counters = j_table["with_counters"]
                assert(type(table.with_counters) is bool)
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


def ignore_template_file(filename):
    """
    Ignore these files in template dir
    """
    pattern = re.compile('^\..*|.*\.cache$|.*~$')
    return pattern.match(filename)

def gen_file_lists(current_dir, gen_dir):
    """
    Generate target files from template; only call once
    """
    files_out = []
    for root, subdirs, files in os.walk(current_dir):
        for filename in files:
            if ignore_template_file(filename):
                continue
            relpath = os.path.relpath(os.path.join(root, filename), templates_dir)
            template_file = relpath
            target_file = os.path.join(gen_dir, relpath)
            files_out.append((template_file, target_file))
    return files_out

def render_all_files(render_dict, gen_dir):
    files = gen_file_lists(templates_dir, gen_dir)
    for template, target in files:
        path = os.path.dirname(target)
        if not os.path.exists(path):
            os.makedirs(path)
        with open(target, "w") as f:
            render_template(f, template, render_dict, templates_dir,
                            prefix = tenjin_prefix)

def _validate_dir(dir_name):
    if not os.path.isdir(dir_name):
        print dir_name, "is not a valid directory"
        sys.exit(1)
    return os.path.abspath(dir_name)

def get_c_type(byte_width):
    if byte_width == 1:
        return "uint8_t"
    elif byte_width == 2:
        return "uint16_t"
    elif byte_width <= 4:
        return "uint32_t"
    else:
        return "uint8_t *"

# key is a Python list of tuples (field_name, match_type, bitwidth)
def gen_match_params(key):
    params = []
    for field, match_type, bitwidth in key:
        bytes_needed = bits_to_bytes(bitwidth)
        params += [(field, bytes_needed)]
        if match_type == MatchType.LPM: params += [(field + "_prefix_length", 2)]
        if match_type == MatchType.TERNARY: params += [(field + "_mask", bytes_needed)]
    return params

def gen_action_params(runtime_data):
    params = []
    for name, bitwidth in runtime_data:
        # for some reason, I was prefixing everything with "action_" originally
        name = "action_" + name
        params += [(name, bits_to_bytes(bitwidth))]
    return params

def bits_to_bytes(bw):
    return (bw + 7) / 8

def get_c_name(name):
    return name.replace(".", "_")

def main():
    load_json(args.json)
    render_dict = {}
    render_dict["p4_prefix"] = args.p4_prefix
    render_dict["pd_prefix"] = "p4_pd_" + args.p4_prefix + "_"
    render_dict["MatchType"] = MatchType
    render_dict["gen_match_params"] = gen_match_params
    render_dict["gen_action_params"] = gen_action_params
    render_dict["bits_to_bytes"] = bits_to_bytes
    render_dict["get_c_type"] = get_c_type
    render_dict["get_c_name"] = get_c_name
    render_dict["tables"] = TABLES
    render_dict["actions"] = ACTIONS
    render_all_files(render_dict, _validate_dir(os.path.join(_THIS_DIR, "gen-cpp")))

if __name__ == '__main__':
    main()

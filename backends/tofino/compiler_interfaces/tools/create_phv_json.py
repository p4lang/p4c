#!/usr/bin/env python3

"""
This script produces a phv.json from an input context.json file.
"""

import json, jsonschema, logging, math, os, sys
from collections import OrderedDict
from .utils import *

if not getattr(sys,'frozen', False):
    # standalone script
    MYPATH = os.path.dirname(__file__)
    SCHEMA_PATH = os.path.join(MYPATH, "../")
    sys.path.append(SCHEMA_PATH)
from schemas.phv_schema import PhvJSONSchema
from schemas.schema_keys import *
from schemas.schema_enum_values import *

# The minimum context.json schema version required.
MINIMUM_CONTEXT_JSON_REQUIRED = "1.7.0"
# The mau.json schema version produced.
PHV_SCHEMA_VERSION = "1.0.3"
PHV_JSON_FILE = "phv.json"


# ----------------------------------------
#  Helpers
# ----------------------------------------


class Access(object):
    def __init__(self, location, location_detail=None,
                 table_name=None, action_name=None,
                 parser_state_name=None, deparser_access_type=None):
        self.location = location
        self.location_detail = location_detail
        self.table_name = table_name
        self.action_name = action_name
        self.parser_state_name = parser_state_name
        self.deparser_access_type = deparser_access_type

    def get_dict(self):
        a = OrderedDict()
        a["location"] = self.location
        if self.location_detail is not None:
            a["location_detail"] = self.location_detail
        if self.table_name is not None:
            a["table_name"] = self.table_name
        if self.action_name is not None:
            a["action_name"] = self.action_name
        if self.parser_state_name is not None:
            a["parser_state_name"] = self.parser_state_name
        if self.deparser_access_type is not None:
            a["deparser_access_type"] = self.deparser_access_type
        return a



# ----------------------------------------
#  Produce PHV JSON
# ----------------------------------------

def get_reads_and_writes(field_name, gress, live_start, live_end, read_dict, write_dict):
    reads = []
    writes = []

    if live_start is not None and live_start == LIVE_PARSER:
        writes.append(Access(LIVE_PARSER, parser_state_name="").get_dict())  # FIXME: How know parse state?

    if (field_name, gress) in read_dict:
        reads.extend([x.get_dict() for x in read_dict[(field_name, gress)]])
    if (field_name, gress) in write_dict:
        writes.extend([x.get_dict() for x in write_dict[(field_name, gress)]])

    if live_end is not None and live_end == LIVE_DEPARSER:
        reads.append(Access(LIVE_DEPARSER, deparser_access_type="pkt").get_dict())  # FIXME: How know deparser access type?

    return reads, writes


def scan_condition_table(table, reads, writes):
    table_name = get_attr(NAME, table)
    gress = get_attr(DIRECTION, table)

    all_stages = set()
    stage_tables = get_attr(STAGE_TABLES, table)
    for s in stage_tables:
        stage_number = get_attr(STAGE_NUMBER, s)
        all_stages.add(stage_number)

    condition_fields = get_attr(CONDITION_FIELDS, table)
    for f in condition_fields:
        fname = get_attr(NAME, f)
        if (fname, gress) not in reads:
            reads[(fname, gress)] = []
        for s in sorted(all_stages):
            reads[(fname, gress)].append(Access(table_name=table_name, location=s, location_detail=XBAR))


def scan_match_table(table, reads, writes):
    table_name = get_attr(NAME, table)
    gress = get_attr(DIRECTION, table)
    match_attributes = get_attr(MATCH_ATTRIBUTES, table)
    match_type = get_attr(MATCH_TYPE, match_attributes)
    all_stages = set()

    if match_type in [ALGORITHMIC_TCAM, CHAINED_LPM]:
        units = get_attr(UNITS, match_attributes)
        for uidx, unit in enumerate(units):
            unit_stage_tables = get_attr(STAGE_TABLES, unit)
            for s in unit_stage_tables:
                stage_number = get_attr(STAGE_NUMBER, s)
                all_stages.add(stage_number)
    else:
        stage_tables_list = get_attr(STAGE_TABLES, match_attributes)
        for s in stage_tables_list:
            stage_number = get_attr(STAGE_NUMBER, s)
            all_stages.add(stage_number)

    match_key_fields = get_attr(MATCH_KEY_FIELDS, table)
    for f in match_key_fields:
        field_name = get_attr(NAME, f)
        if (field_name, gress) not in reads:
            reads[(field_name, gress)] = []
        for s in sorted(all_stages):
            reads[(field_name, gress)].append(Access(table_name=table_name, location=s, location_detail=XBAR))


    def _add_access(some_attr, action_name, which_access, detail=VLIW):
        if some_attr is not None:
            fname = get_attr(NAME, some_attr)
            the_type = get_attr(TYPE, some_attr)
            if the_type == PHV:
                if (fname, gress) not in which_access:
                    which_access[(fname, gress)] = []
                for s in sorted(all_stages):
                    which_access[(fname, gress)].append(Access(table_name=table_name, action_name=action_name,
                                                               location=s, location_detail=detail))

    def _add_stateful_access(some_attr, action_name, which_access, detail=XBAR):
        if some_attr is not None:
            for op_type, op_value in [(OPERAND_1_TYPE, OPERAND_1_VALUE), (OPERAND_2_TYPE, OPERAND_2_VALUE)]:
                the_type = get_optional_attr(op_type, some_attr)
                if the_type is not None and the_type == PHV:
                    the_value = get_optional_attr(op_value, some_attr)
                    if the_value is not None:
                        if (the_value, gress) not in which_access:
                            which_access[(the_value, gress)] = []
                        for s in sorted(all_stages):
                            which_access[(the_value, gress)].append(Access(table_name=table_name, action_name=action_name,
                                                                           location=s, location_detail=detail))

    actions = get_attr(ACTIONS, table)
    for a in actions:
        action_name = get_attr(NAME, a)
        primitives = get_attr(PRIMITIVES, a)
        for p in primitives:
            _add_access(get_optional_attr(DST, p), action_name, writes)
            _add_access(get_optional_attr(SRC1, p), action_name, reads)
            _add_access(get_optional_attr(SRC2, p), action_name, reads)
            _add_access(get_optional_attr(SRC3, p), action_name, reads)
            _add_access(get_optional_attr(IDX, p), action_name, reads, XBAR)
            _add_access(get_optional_attr(COND, p), action_name, reads)

            hash_inputs = get_optional_attr(HASH_INPUTS, p)
            if hash_inputs is not None:
                for fname in hash_inputs:
                    if (fname, gress) not in reads:
                        reads[(fname, gress)] = []
                    for s in sorted(all_stages):
                        reads[(fname, gress)].append(Access(table_name=table_name, action_name=action_name,
                                                        location=s, location_detail=XBAR))

            stateful_alu_details = get_optional_attr(STATEFUL_ALU_DETAILS, p)
            if stateful_alu_details is not None:
                _add_stateful_access(get_optional_attr(CONDITION_HI, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(CONDITION_LO, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_LO_1_PREDICATE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_LO_1_VALUE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_LO_2_PREDICATE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_LO_2_VALUE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_HI_1_PREDICATE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_HI_1_VALUE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_HI_2_PREDICATE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(UPDATE_HI_2_VALUE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(OUTPUT_PREDICATE, stateful_alu_details), action_name, reads)
                _add_stateful_access(get_optional_attr(OUTPUT_VALUE, stateful_alu_details), action_name, reads)

                _add_stateful_access(get_optional_attr(OUTPUT_DST, stateful_alu_details), action_name, writes, VLIW)


def populate_reads_and_writes(context):
    # (table name, gress) to list of Access objects
    reads = {}
    writes = {}

    tables = get_attr(TABLES, context)
    for t in tables:
        table_type = get_attr(TABLE_TYPE, t)
        if table_type == MATCH:
            scan_match_table(t, reads, writes)
        elif table_type == CONDITION:
            scan_condition_table(t, reads, writes)

    return reads, writes



def produce_containers_node(context):
    containers = []

    reads, writes = populate_reads_and_writes(context)

    phv_allocation = get_attr(PHV_ALLOCATION, context)
    for phv_stage in phv_allocation:
        stage_number = get_attr(STAGE_NUMBER, phv_stage)
        for g in [INGRESS, EGRESS]:
            gress_list = get_attr(g, phv_stage)
            for container in gress_list:
                phv_number = get_attr(PHV_NUMBER, container)
                container_type = get_attr(CONTAINER_TYPE, container)
                word_bit_width = get_attr(WORD_BIT_WIDTH, container)
                records = get_attr(RECORDS, container)

                container_dict = OrderedDict()
                container_dict["phv_number"] = phv_number
                container_dict["bit_width"] = word_bit_width
                container_dict["phv_mau_group_number"] = 0
                container_dict["phv_deparser_group_number"] = 0
                container_dict["container_type"] = container_type

                container_records =[]
                for r in records:
                    is_pov = get_attr(IS_POV, r)
                    field_name = get_attr(FIELD_NAME, r)
                    field_lsb = get_attr(FIELD_LSB, r)
                    field_msb = get_attr(FIELD_MSB, r)
                    phv_lsb = get_attr(PHV_LSB, r)
                    phv_msb = get_attr(PHV_MSB, r)
                    # is_compiler_generated = get_attr(IS_COMPILER_GENERATED, r)
                    live_start = get_optional_attr(LIVE_START, r)
                    live_end = get_optional_attr(LIVE_END, r)

                    rec = OrderedDict()
                    rec["field_name"] = field_name
                    rec["field_class"] = "pkt"  # FIXME: Don't know how to get the value from context.json.
                    rec["field_msb"] = field_msb
                    rec["field_lsb"] = field_lsb
                    rec["phv_msb"] = phv_msb
                    rec["phv_lsb"] = phv_lsb
                    rec["gress"] = g
                    rec["reads"], rec["writes"] = get_reads_and_writes(field_name, g, live_start, live_end, reads, writes)
                    if has_attr(FORMAT_TYPE, r):
                        rec["format_type"] = get_attr(FORMAT_TYPE, r)

                    container_records.append(rec)

                container_dict["records"] = container_records
                containers.append(container_dict)

        break  # FIXME: Only look at stage 0 for now.

    containers.sort(key=lambda x: x["phv_number"])
    return containers


def produce_resources_node(context):
    schema_version = get_attr(SCHEMA_VERSION, context)
    target = get_attr_first_available(TARGET_KEY, context, "1.6.4", schema_version)

    resources = []
    grp_id = 0
    mau_grp_containers = 16
    max_mau_containers = 224
    if target is not None and target != TARGET.TOFINO:
        mau_grp_containers = 20
        max_mau_containers = 280

    def _get_bw_for_addr(addr, target=TARGET.TOFINO):
        if target == TARGET.TOFINO or target is None:
            if addr < 64:
                return 32
            elif addr < 128:
                return 8
            elif addr < 224:
                return 16
            elif addr < 288:
                return 32
            elif addr < 320:
                return 8
            else:
                return 16
        elif target is not None and target != TARGET.TOFINO:
            if 0 <= addr < 80:
                return 32
            elif 80 <= addr < 160:
                return 8
            else:
                return 16
        else:
            print_error_and_exit("Unknown target '%s'" % str(target))

    for start_addr in range(0, max_mau_containers, mau_grp_containers):
        grp = OrderedDict()
        grp["group_type"] = MAU
        grp["bit_width"] = _get_bw_for_addr(start_addr, target)
        grp["num_containers"] = mau_grp_containers
        grp["addresses"] = list(range(start_addr, start_addr + mau_grp_containers))
        resources.append(grp)
        grp_id += 1
    if target == TARGET.TOFINO:
        for start_addr in range(256, 368, 16):
            grp = OrderedDict()
            grp["group_type"] = TAGALONG
            grp["bit_width"] = _get_bw_for_addr(start_addr)
            grp["num_containers"] = 16
            grp["addresses"] = list(range(start_addr, start_addr + 16))
            resources.append(grp)
            grp_id += 1
    return resources


def produce_phv_json(source, output):
    # Load JSON and make sure versions are correct
    json_file = open(source, 'r')
    try:
        context = json.load(json_file)
    except:
        json_file.close()
        context = {}
        print_error_and_exit("Unable to open JSON file '%s'" % str(source))

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_CONTEXT_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported context.json schema version %s.\n" % str(schema_version)
        error_msg += "Required minimum version is: %s" % str(MINIMUM_CONTEXT_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    target = get_attr_first_available(TARGET_KEY, context, "1.6.4", schema_version)
    if target is None:  # For old versions of schema, assume Tofino
        target = TARGET.TOFINO

    # Create phv.json dictionary
    phv_json = OrderedDict()
    phv_json["build_date"] = get_attr(BUILD_DATE, context)
    phv_json["program_name"] = get_attr(PROGRAM_NAME, context)
    phv_json["run_id"] = get_attr(RUN_ID, context)
    phv_json["compiler_version"] = get_attr(COMPILER_VERSION, context)
    phv_json["schema_version"] = PHV_SCHEMA_VERSION
    phv_json["target"] = target

    phv_json["nStages"] = len(get_attr(PHV_ALLOCATION, context))
    phv_json["containers"] = produce_containers_node(context)
    phv_json["resources"] = produce_resources_node(context)
    phv_json["structures"] = []  # FIXME
    phv_json["pov_structure"] = []  # FIXME

    # Tidy up and validate
    json_file.close()

    phv_schema = PhvJSONSchema.get_schema()
    cpath = os.path.join(output, PHV_JSON_FILE)
    with open(cpath, "w") as cfile:
        json.dump(phv_json, cfile, indent=4)
    jsonschema.validate(phv_json, phv_schema)
    return SUCCESS, "ok"


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument('source', metavar='source', type=str,
                        help='The input context.json source file to use.')
    parser.add_argument('--output', '-o', type=str, action="store", default=".",
                        help="The output directory to output mau.json.")
    args = parser.parse_args()

    try:
        if not os.path.exists(args.output):
            os.mkdir(args.output)
    except:
        print_error_and_exit("Unable to create directory %s." % str(args.output))

    return_code = SUCCESS
    err_msg = ""
    try:
        return_code, err_msg = produce_phv_json(args.source, args.output)
    except:
        print_error_and_exit("Unable to create %s." % PHV_JSON_FILE)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

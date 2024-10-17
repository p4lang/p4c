#!/usr/bin/env python3

"""
This script produces a mau.json from an input context.json file.
"""

import json, jsonschema, logging, math, os, sys
from collections import OrderedDict
from .utils import *

if not getattr(sys,'frozen', False):
    # standalone script
    MYPATH = os.path.dirname(__file__)
    SCHEMA_PATH = os.path.join(MYPATH, "../")
    sys.path.append(SCHEMA_PATH)
from schemas.mau_schema import MauJSONSchema
from schemas.schema_keys import *
from schemas.schema_enum_values import *

# The minimum context.json schema version required.
MINIMUM_CONTEXT_JSON_VERSION = "1.7.0"
# The mau.json schema version produced.
MAU_SCHEMA_VERSION = "1.0.0"
MAU_JSON_FILE = "mau.json"


# ----------------------------------------
#  Helpers
# ----------------------------------------
REF_TYPE_TO_TABLE_TYPE = {ACTION_DATA_TABLE_REFS: ACTION,
                          METER_TABLE_REFS: METER,
                          SELECTION_TABLE_REFS: SELECTION,
                          STATEFUL_TABLE_REFS: STATEFUL,
                          STATISTICS_TABLE_REFS: STATISTICS}


def find_table_node(table_name, table_handle, table_type, context):
    tables = get_attr(TABLES, context)
    for t in tables:
        name = get_attr(NAME, t)
        handle = get_attr(HANDLE, t)
        ttype = get_attr(TABLE_TYPE, t)
        if name == table_name and ttype == table_type:
            return t
        if handle is not None and handle == table_handle and ttype == table_type:
            return t
    print_error_and_exit("Unable to find table '%s' (%s) with handle '%s' in '%s' node." % (table_name, table_type, str(table_handle), TABLES))



# ----------------------------------------
#  Produce MAU JSON
# ----------------------------------------

def get_match_stage_tables(match_table):
    match_attr = get_attr(MATCH_ATTRIBUTES, match_table)
    match_type = get_attr(MATCH_TYPE, match_attr)
    stage_tables_list = []
    if match_type in [ALGORITHMIC_TCAM, CHAINED_LPM]:
        units = get_attr(UNITS, match_attr)
        for u in units:
            stage_tables_list.extend(get_match_stage_tables(u))
    elif match_type == ALGORITHMIC_LPM:
        atcam_table = get_attr(ATCAM_TABLE, match_attr)
        match_attr2 = get_attr(MATCH_ATTRIBUTES, atcam_table)
        units = get_attr(UNITS, match_attr2)
        for u in units:
            stage_tables_list.extend(get_match_stage_tables(u))
    else:
        stage_tables_list = get_attr(STAGE_TABLES, match_attr)
    return stage_tables_list


def get_entries_allocated(match_table):
    stage_tables = get_match_stage_tables(match_table)
    entries = 0
    for s in stage_tables:
        entries += get_attr(SIZE, s)
    return entries


def get_pack_format(match_stage_table, match_table):
    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)
    pack_format = []
    if table_type == MATCH_WITH_NO_KEY:  # No pack format expected
        pass
    elif table_type in [HASH_MATCH, PROXY_HASH_MATCH, DIRECT_MATCH]:
        ways = get_attr(WAYS, match_stage_table)
        if len(ways) > 0:
            pack_format = get_attr(PACK_FORMAT, ways[0])
    else:
        pack_format = get_attr(PACK_FORMAT, match_stage_table)
        if table_type == TERNARY_MATCH:
            pack_format = convert_ternary_pack_format(pack_format)
    return pack_format


def get_action_parameters_json(match_table):
    actions = []
    actions_list = get_attr(ACTIONS, match_table)
    for a in actions_list:
        adict = OrderedDict()
        adict["action_name"] = get_attr(NAME, a)
        params = []
        p4_params = get_attr(P4_PARAMETERS, a)
        for p in p4_params:
            pdict = OrderedDict()
            pdict["name"] = get_attr(NAME, p)
            pdict["bit_width"] = get_attr(BIT_WIDTH, p)
            params.append(pdict)
        adict["parameters"] = params
        actions.append(adict)
    return actions


def get_match_key_fields_json(match_key_fields):
    lookup_types = set()
    keys = []
    for mdict in match_key_fields:
        kdict = OrderedDict()
        for attr in [NAME, START_BIT, BIT_WIDTH]:
            value = get_attr(attr, mdict)
            kdict[attr] = value
        kdict["lookup_type"] = get_attr(MATCH_TYPE, mdict)
        lookup_types.add(kdict["lookup_type"])
        keys.append(kdict)
    return list(lookup_types), keys


def get_parameter_map(action, action_stage_table):
    # FIXME: context.json does not have the information needed by this function.
    p4_params = get_attr(P4_PARAMETERS, action)

    param_map = []

    # for p in p4_params:
    #     pdict = OrderedDict()
    #     pdict["name"] = get_attr(NAME, p)
    #     pdict["bit_width"] = get_attr(BIT_WIDTH, p)  # FIXME: Don't know how this parameter has been split
    #     pdict["start_bit"] = 0  # FIXME: Don't know how this parameter has been split
    #     pdict["location"] = "action_data"  # FIXME: placeholder
    #     pdict["action_data_slot_size"] = 8  # FIXME: placeholder
    #     pdict["action_slot_number"] = 0  # FIXME: placeholder
    #     param_map.append(pdict)

    return param_map


def get_stage_action_formats(match_stage_table, match_table, context):
    stage_number = get_attr(STAGE_NUMBER, match_stage_table)
    adt_refs = get_attr(ACTION_DATA_TABLE_REFS, match_table)

    stage_action_formats = []

    for ref in adt_refs:
        name = get_attr(NAME, ref)
        handle = get_attr(HANDLE, ref)
        action_table = find_table_node(name, handle, REF_TYPE_TO_TABLE_TYPE[ACTION_DATA_TABLE_REFS], context)
        action_stage_tables = get_attr(STAGE_TABLES, action_table)
        actions_list = get_attr(ACTIONS, action_table)

        found_stage = False
        for s in action_stage_tables:
            action_stage_num = get_attr(STAGE_NUMBER, s)
            if stage_number != action_stage_num:
                continue

            found_stage = True
            action_pack_formats = get_attr(PACK_FORMAT, s)

            for a in actions_list:
                action_name = get_attr(NAME, a)
                action_handle = get_attr(HANDLE, a)

                found = False

                for apf in action_pack_formats:
                    pf_handle = get_attr(ACTION_HANDLE, apf)
                    if action_handle != pf_handle:  # Find appropriate pack format
                        continue

                    found = True
                    entries = get_stage_action_format(apf)

                    aformat = OrderedDict()
                    aformat["name"] = action_name
                    aformat["action_format"] = {"entries": entries}
                    aformat["parameter_map"] = get_parameter_map(a, s)  # FIXME: Information does not exist in context.json

                    stage_action_formats.append(aformat)

                # if not found:
                #     print_error_and_exit("Unable to find action format for action %s in stage %s" % (action_name, stage_number))
    return stage_action_formats


def get_stage_action_format(pack_format):
    entries_list = []

    entries = get_attr(ENTRIES, pack_format)
    memory_word_width = get_attr(MEMORY_WORD_WIDTH, pack_format)

    for e in entries:
        edict = OrderedDict()
        edict["entry_number"] = get_attr(ENTRY_NUMBER, e)
        fields = get_attr(FIELDS, e)
        fields_list = []
        for f in fields:
            fdict = OrderedDict()
            fdict["name"] = get_attr(FIELD_NAME, f)
            fdict["bit_width"] = get_attr(FIELD_WIDTH, f)
            fdict["start_bit"] = get_attr(START_BIT, f)

            lsb_mem_word_idx = get_attr(LSB_MEM_WORD_IDX, f)
            lsb_mem_word_offset = get_attr(LSB_MEM_WORD_OFFSET, f)
            mem_start_bit = (memory_word_width * lsb_mem_word_idx) + lsb_mem_word_offset
            fdict["memory_start_bit"] = mem_start_bit
            fields_list.append(fdict)

        edict["fields"] = fields_list
        entries_list.append(edict)

    return entries_list


def convert_ternary_pack_format(pack_format):
    """
    context.json publishes ternary match formats to include tcam payload and parity,
    making the tcam word 47 bits.  This is not desirable for MAU characterization, since
    the actual data bits are only 44 bits.  This function strips away those fields.
    """
    new_pack_format = []
    for p in pack_format:
        new_p = OrderedDict()
        new_p["table_word_width"] = -1  # Will update below
        new_p["memory_word_width"] = 44
        new_p["entries_per_table_word"] = get_attr(ENTRIES_PER_TABLE_WORD, p)
        new_p["number_memory_units_per_table_word"] = get_attr(NUMBER_MEMORY_UNITS_PER_TABLE_WORD, p)
        new_p["table_word_width"] = 44 * new_p["number_memory_units_per_table_word"]

        entries = get_attr(ENTRIES, p)
        new_entries = []
        for e in entries:
            new_e = OrderedDict()
            new_e["entry_number"] = get_attr(ENTRY_NUMBER, e)
            new_fields = []
            fields = get_attr(FIELDS, e)
            for f in fields:
                src = get_attr(SOURCE, f)

                if src in [PARITY, PAYLOAD]:  # Skip these fields
                    continue

                fdict = OrderedDict()
                fdict["field_name"] = get_attr(FIELD_NAME, f)
                fdict["field_width"] = get_attr(FIELD_WIDTH, f)
                fdict["lsb_mem_word_idx"] = get_attr(LSB_MEM_WORD_IDX, f)
                fdict["msb_mem_word_idx"] = get_attr(MSB_MEM_WORD_IDX, f)
                fdict["lsb_mem_word_offset"] = get_attr(LSB_MEM_WORD_OFFSET, f)
                fdict["start_bit"] = get_attr(START_BIT, f)
                fdict["source"] = src
                # fdict["range"] = get_attr(FIELD_NAME, f)  # Not needed for this file
                new_fields.append(fdict)

            new_e["fields"] = new_fields
            new_entries.append(new_e)

        new_p["entries"] = new_entries
        new_pack_format.append(new_p)

    return new_pack_format


def get_stage_match_format(match_stage_table, match_table):
    mdict = OrderedDict()
    entries_list = []
    pack_format = get_pack_format(match_stage_table, match_table)
    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)
    mytname = get_attr(NAME, match_table)

    if table_type == HASH_ACTION:  # hash-action tables have no entries
        mdict['entries'] = []
        return mdict

    for p in pack_format:
        memory_word_width = get_attr(MEMORY_WORD_WIDTH, p)
        entries = get_attr(ENTRIES, p)
        for e in entries:
            edict = OrderedDict()
            edict["entry_number"] = get_attr(ENTRY_NUMBER, e)
            fields = get_attr(FIELDS, e)
            fields_list = []
            for f in fields:
                fdict = OrderedDict()
                fdict["name"] = get_attr(FIELD_NAME, f)
                fdict["bit_width"] = get_attr(FIELD_WIDTH, f)
                fdict["start_bit"] = get_attr(START_BIT, f)
                src = get_attr(SOURCE, f)
                if src == ZERO:  # Skip padding
                    continue
                lsb_mem_word_idx = get_attr(LSB_MEM_WORD_IDX, f)
                lsb_mem_word_offset = get_attr(LSB_MEM_WORD_OFFSET, f)
                mem_start_bit = (memory_word_width * lsb_mem_word_idx) + lsb_mem_word_offset
                if table_type == TERNARY_MATCH:
                    mem_start_bit -= 1  # Strip payload bit

                fdict["memory_start_bit"] = mem_start_bit
                fields_list.append(fdict)
            edict["fields"] = fields_list
            entries_list.append(edict)

        break  # Only need to look at first pack format

    mdict["entries"] = entries_list
    return mdict


def get_entry_bit_width(pack_format, stage_table_type):
    match_bits = 0
    entries = get_attr(ENTRIES, pack_format)
    for e in entries:
        fields = get_attr(FIELDS, e)
        for f in fields:
            source = get_attr(SOURCE, f)
            # skip padding for ram-based match tables
            if source == ZERO and stage_table_type in [ALGORITHMIC_TCAM_MATCH, DIRECT_MATCH,
                                                       HASH_WAY, HASH_MATCH, PROXY_HASH_MATCH]:
                continue
            match_bits += get_attr(FIELD_WIDTH, f)
        break  # Only care about the first entry
    return match_bits


def get_overhead_field_size(source_enum_value, pack_format_list):
    for p in pack_format_list:
        entries = get_optional_attr(ENTRIES, p)  # This is optional for hash-action
        if entries is not None:
            for e in entries:
                fields = get_attr(FIELDS, e)
                for f in fields:
                    source = get_attr(SOURCE, f)
                    if source == source_enum_value:
                        return get_attr(FIELD_WIDTH, f)
                break  # only need to look at one entry
            break  # Only need to look at first pack format
    return 0  # Didn't find it


def get_allocated_overhead_size(pack_format):
    oh_size = 0
    for oh_source in ALL_OH:
        oh_size += get_overhead_field_size(oh_source, pack_format)
    return oh_size


def get_max_parameter_size_in_overhead(match_stage_table, match_table, context):
    """
    Scan actions in match table, see what's in match overhead.  The largest size becomes "ideal."
    This is based on the final allocation, not necessarily what is programmatically "ideal."
    """
    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)

    action_format = []
    if table_type == TERNARY_MATCH:
        tind_stage_table = get_attr(TERNARY_INDIRECTION_STAGE_TABLE, match_stage_table)
        action_format = get_attr(ACTION_FORMAT, tind_stage_table)
    elif table_type == PHASE_0_MATCH:
        params_per_action_list = get_action_parameters_json(match_table)
        max_param_bw = 0
        for action_dict in params_per_action_list:
            action_param_bw = 0
            for param_dict in action_dict["parameters"]:
                action_param_bw += param_dict["bit_width"]
            max_param_bw = max(action_param_bw, max_param_bw)
        return max_param_bw
    else:
        try:
            action_format = get_attr(ACTION_FORMAT, match_stage_table)
        except:
            print("Unable to find action format for table type %s" % table_type)

    max_param_size = 0
    for af in action_format:
        immediate_fields = get_attr(IMMEDIATE_FIELDS, af)
        local_param_size = 0
        for imm in immediate_fields:
            dest_width = get_attr(DEST_WIDTH, imm)
            local_param_size += dest_width
        max_param_size = max(local_param_size, max_param_size)
    return max_param_size


def get_ideal_match_entries(match_stage_table, match_table, ideal_overhead_imm_bw):
    # For ideal entry packing, we only use match key field bit width, no overhead.
    ideal_entries = 0
    ideal_mem_blocks_wide = 0
    ideal_util = 0

    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)
    pack_format = get_pack_format(match_stage_table, match_table)

    mem_width = 1
    for p in pack_format:
        mem_width = get_attr(MEMORY_WORD_WIDTH, p)
        break  # Only care about first instance of packet format

    p4_lookup_types, p4_match_fields = get_match_key_fields_json(get_attr(MATCH_KEY_FIELDS, match_table))
    match_bits = 0
    for mdict in p4_match_fields:
        match_bits += mdict["bit_width"]

    if table_type != TERNARY_MATCH:
        # Ideal packing considers that ideal action data immediate overhead is also included.
        # (The action parameters have to go somewhere.)
        # For ternary tables, this is in TIND.
        match_bits += ideal_overhead_imm_bw

    max_entries = 1  # ternary match cannot pack more than one entry
    if match_bits > 0 and table_type != TERNARY_MATCH:
        max_wide_width = 128 * 8  # sram width * number of rows
        max_entries = max_wide_width // match_bits
        max_entries = min(max_entries, 8 * 5)  # 8 rows * 5 entries per row
        if table_type == ALGORITHMIC_TCAM_MATCH:
            max_entries = min(max_entries, 5)  # alg tcam can't do unlimited chaining

    if table_type == PHASE_0_MATCH:
        max_entries = 1

    for num_entries in range(1, max_entries + 1):
        mem_units = int(math.ceil((num_entries * match_bits) / float(mem_width)))

        if num_entries > (mem_units * 5):  # Invalid packing
            continue

        total_mem = mem_units * mem_width
        if total_mem > 0:
            util = (num_entries * match_bits) / float(total_mem)
        else:
            util = 0

        if ideal_entries == 0 or util > ideal_util:
            ideal_entries = num_entries
            ideal_mem_blocks_wide = mem_units
            ideal_util = util

    return ideal_entries, ideal_mem_blocks_wide * mem_width


def get_match_memory(match_stage_table, match_table, context, entries_so_far):
    memories = []

    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)
    table_entries = get_attr(SIZE, match_table)
    mem_allocs = []
    mem_type = SRAM
    if table_type in [MATCH_WITH_NO_KEY, HASH_ACTION]:  # No match memory expected
        return []
    elif table_type in [ALGORITHMIC_TCAM_MATCH]:
        mem_allocs_list = get_attr(MEMORY_RESOURCE_ALLOCATION, match_stage_table)
        for m in mem_allocs_list:
            mem_allocs.append(m)
    elif table_type in [HASH_MATCH, PROXY_HASH_MATCH, DIRECT_MATCH]:
        ways = get_attr(WAYS, match_stage_table)
        for w in ways:
            mem_allocs.append(get_attr(MEMORY_RESOURCE_ALLOCATION, w))
    else:
        mem_alloc = get_attr(MEMORY_RESOURCE_ALLOCATION, match_stage_table)
        if mem_alloc is not None:  # Can be None for keyless ternary tables (always miss), so no memory allocated
            mem_allocs.append(mem_alloc)
            try:
                mem_type = get_attr(MEMORY_TYPE, mem_alloc)
            except:
                print("no mem alloc for table type %s" % table_type)

    p4_lookup_types, p4_match_fields = get_match_key_fields_json(get_attr(MATCH_KEY_FIELDS, match_table))
    ideal_entry_bits = 0
    for mdict in p4_match_fields:
        ideal_entry_bits += mdict["bit_width"]

    table_word_width = 0
    memory_word_width = 0
    entries_per_table_word = 0
    allocated_match_bits = 0
    num_mems = 0

    pack_format = get_pack_format(match_stage_table, match_table)

    for p in pack_format:
        table_word_width = get_attr(TABLE_WORD_WIDTH, p)
        memory_word_width = get_attr(MEMORY_WORD_WIDTH, p)
        entries_per_table_word = get_attr(ENTRIES_PER_TABLE_WORD, p)
        allocated_match_bits = get_entry_bit_width(p, table_type)
        break  # Only care about first pack format

    # Adjust ideal based on overhead requirements
    if table_type == TERNARY_MATCH:
        ver_size = get_overhead_field_size(OH_VERSION, pack_format)
        ideal_entry_bits += ver_size
        if ver_size > 0:  # We include two wasted bits that could be used for container valid.
            ideal_entry_bits += 2
    elif table_type != PHASE_0_MATCH:
        # Ideal packing considers that ideal action data immediate overhead is also included.
        # (The action parameters have to go somewhere.)
        # For ternary tables, this is in TIND.
        ideal_entry_bits += get_allocated_overhead_size(pack_format)

    if mem_type == INGRESS_BUFFER:  # FIXME: Inconsistency between schemas
        mem_type = BUF

    for mem_alloc in mem_allocs:  # Multiple items if iterating through ways
        if mem_alloc is None:
            print_error_and_exit("Unexpected null memory resource allocation for table %s." % get_attr(NAME, match_table))
        mem_units_and_vpns = get_attr(MEMORY_UNITS_AND_VPNS, mem_alloc)
        for memdict in mem_units_and_vpns:
            memory_units = get_attr(MEMORY_UNITS, memdict)
            num_mems += len(memory_units)

    mem_elem = OrderedDict()
    mem_elem["memory_type"] = mem_type
    mem_elem["table_word_width"] = table_word_width
    mem_elem["memory_word_width"] = memory_word_width
    mem_elem["entries_per_table_word"] = entries_per_table_word
    mem_elem["table_type"] = MATCH
    mem_elem["num_memories"] = num_mems

    entries_allocated = get_attr(SIZE, match_stage_table)
    mem_elem["entries_requested"] = min(entries_allocated, max(0, table_entries - entries_so_far))
    mem_elem["entries_allocated"] = entries_allocated

    mem_elem["imm_bit_width_in_overhead_requested"] = get_max_parameter_size_in_overhead(match_stage_table, match_table, context)

    if table_type == TERNARY_MATCH:
        tind_stage_table = get_attr(TERNARY_INDIRECTION_STAGE_TABLE, match_stage_table)
        tind_pack_format = get_pack_format(tind_stage_table, match_table)
        mem_elem["imm_bit_width_in_overhead_allocated"] = get_overhead_field_size(OH_IMMEDIATE, tind_pack_format)
    elif table_type == PHASE_0_MATCH:
        mem_elem["imm_bit_width_in_overhead_allocated"] = 0
    else:
        mem_elem["imm_bit_width_in_overhead_allocated"] = get_overhead_field_size(OH_IMMEDIATE, pack_format)

    mem_elem["entry_bit_width_requested"] = ideal_entry_bits
    mem_elem["entry_bit_width_allocated"] = allocated_match_bits

    mem_elem["ideal_entries_per_table_word"], mem_elem["ideal_table_word_bit_width"] = \
        get_ideal_match_entries(match_stage_table, match_table, mem_elem["imm_bit_width_in_overhead_requested"])

    memories.append(mem_elem)

    return memories


def get_tind_memory(match_stage_table, match_table, context):
    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)
    if table_type != TERNARY_MATCH:
        return []

    tind_stage_table = get_attr(TERNARY_INDIRECTION_STAGE_TABLE, match_stage_table)
    pack_format = get_pack_format(tind_stage_table, match_table)

    table_word_width = 0
    memory_word_width = 0
    entries_per_table_word = 0
    entry_bit_width = 0
    num_mems = 0

    for p in pack_format:
        table_word_width = get_attr(TABLE_WORD_WIDTH, p)
        memory_word_width = get_attr(MEMORY_WORD_WIDTH, p)
        entries_per_table_word = get_attr(ENTRIES_PER_TABLE_WORD, p)
        entry_bit_width = get_entry_bit_width(p, table_type)
        break  # Only care about first pack format

    mem_alloc = get_attr(MEMORY_RESOURCE_ALLOCATION, tind_stage_table)
    if mem_alloc is None:  # No memory was needed
        return []

    mem_units_and_vpns = get_attr(MEMORY_UNITS_AND_VPNS, mem_alloc)
    for memdict in mem_units_and_vpns:
        memory_units = get_attr(MEMORY_UNITS, memdict)
        num_mems += len(memory_units)

    mem_elem = OrderedDict()
    mem_elem["memory_type"] = SRAM
    mem_elem["table_word_width"] = table_word_width
    mem_elem["memory_word_width"] = memory_word_width
    mem_elem["entries_per_table_word"] = entries_per_table_word
    mem_elem["table_type"] = TIND
    mem_elem["num_memories"] = num_mems

    tind_entry_size = get_allocated_overhead_size(pack_format)

    mem_elem["entry_bit_width_requested"] = tind_entry_size
    mem_elem["entry_bit_width_allocated"] = entry_bit_width

    return [mem_elem]


def get_side_effect_memory(match_stage_table, match_table, context, stage_table_idx=None):

    memories = []

    stage_number = get_attr(STAGE_NUMBER, match_stage_table)

    for ref_type in ALL_REF_TYPES:

        refs = get_attr(ref_type, match_table)
        for ref in refs:
            name = get_attr(NAME, ref)
            handle = get_attr(HANDLE, ref)
            p4_table = find_table_node(name, handle, REF_TYPE_TO_TABLE_TYPE[ref_type], context)

            stage_tables = get_attr(STAGE_TABLES, p4_table)

            for sidx, s in enumerate(stage_tables):
                # For things like algorithmic tcam, only get the stage table associated with an index.
                if stage_table_idx is not None and sidx != stage_table_idx:
                    continue

                st_num = get_attr(STAGE_NUMBER, s)
                table_type = get_attr(STAGE_TABLE_TYPE, s)
                if stage_number != st_num:
                    continue

                pack_format = get_pack_format(s, match_table)

                table_word_width = 0
                memory_word_width = 0
                entries_per_table_word = 0
                entry_bit_width = 0
                num_mems = 0

                mem_alloc = get_attr(MEMORY_RESOURCE_ALLOCATION, s)
                mem_units_and_vpns = get_attr(MEMORY_UNITS_AND_VPNS, mem_alloc)
                for memdict in mem_units_and_vpns:
                    memory_units = get_attr(MEMORY_UNITS, memdict)
                    num_mems += len(memory_units)

                # The spare bank (for synth2port) is not part of mem_units_and_vpns
                if has_attr(SPARE_BANK_MEMORY_UNIT, mem_alloc):
                    num_mems += 1

                for p in pack_format:
                    table_word_width = get_attr(TABLE_WORD_WIDTH, p)
                    memory_word_width = get_attr(MEMORY_WORD_WIDTH, p)
                    entries_per_table_word = get_attr(ENTRIES_PER_TABLE_WORD, p)
                    if ref_type == ACTION_DATA_TABLE_REFS:
                        entry_bit_width = max(entry_bit_width, get_entry_bit_width(p, table_type))
                    break  # Only care about first pack format

                mem_elem = OrderedDict()
                mem_elem["memory_type"] = SRAM
                mem_elem["table_word_width"] = table_word_width
                mem_elem["memory_word_width"] = memory_word_width
                mem_elem["entries_per_table_word"] = entries_per_table_word
                mem_elem["table_type"] = REF_TYPE_TO_TABLE_TYPE[ref_type]
                mem_elem["num_memories"] = num_mems

                if ref_type == ACTION_DATA_TABLE_REFS:
                    params_per_action_list = get_action_parameters_json(match_table)
                    max_param_bw = 0
                    for action_dict in params_per_action_list:
                        action_param_bw = 0
                        for param_dict in action_dict["parameters"]:
                            action_param_bw += param_dict["bit_width"]
                        max_param_bw = max(action_param_bw, max_param_bw)

                    match_pack_format = get_pack_format(match_stage_table, match_table)
                    imm_bw = get_overhead_field_size(OH_IMMEDIATE, match_pack_format)
                    mem_elem["entry_bit_width_requested"] = max(0, max_param_bw - imm_bw)
                    mem_elem["entry_bit_width_allocated"] = entry_bit_width

                    if max_param_bw >=128:
                        mem_elem["ideal_entries_per_table_word"] = 1
                        mem_elem["ideal_table_word_bit_width"] = next_power_2(max_param_bw)
                    else:
                        if max_param_bw == 0:
                            mem_elem["ideal_entries_per_table_word"] = 1
                        else:
                            mem_elem["ideal_entries_per_table_word"] = 128 // next_power_2(max_param_bw)
                        mem_elem["ideal_table_word_bit_width"] = 128

                memories.append(mem_elem)

    return memories


def get_stage_memories(match_stage_table, match_table, context, entries_so_far, stage_table_idx=None):
    memories = []
    memories.extend(get_match_memory(match_stage_table, match_table, context, entries_so_far))
    memories.extend(get_tind_memory(match_stage_table, match_table, context))
    memories.extend(get_side_effect_memory(match_stage_table, match_table, context, stage_table_idx))
    return memories


def get_overhead_fields(match_stage_table, match_table):
    overhead = []
    all_oh_copy = [x for x in ALL_OH]
    all_oh_copy.remove(OH_VERSION)  # Remove this so diff is easier, and only should be added for non tcam tables

    table_type = get_attr(STAGE_TABLE_TYPE, match_stage_table)
    pack_format = get_pack_format(match_stage_table, match_table)
    if table_type == HASH_ACTION:  # hash-action has no match overhead
        return []
    if table_type == TERNARY_MATCH:
        tind_stage_table = get_attr(TERNARY_INDIRECTION_STAGE_TABLE, match_stage_table)
        pack_format = get_pack_format(tind_stage_table, match_table)
    ver_bw = get_overhead_field_size(OH_VERSION, pack_format)
    for oh_source in all_oh_copy:
        oh_size = get_overhead_field_size(oh_source, pack_format)
        if oh_size > 0:
            odict = OrderedDict()
            odict["name"] = OVERHEAD_SOURCE_TO_NAME[oh_source]
            odict["bit_width"] = oh_size
            overhead.append(odict)
    if ver_bw > 0 and table_type != TERNARY_MATCH:
        odict = OrderedDict()
        odict["name"] = OVERHEAD_SOURCE_TO_NAME[OH_VERSION_VALID]
        odict["bit_width"] = ver_bw
        overhead.append(odict)

    return overhead


def get_stage_allocation(table, context):
    stage_list = []
    stage_tables = get_match_stage_tables(table)
    entries_so_far = 0
    for s in stage_tables:
        sdict = OrderedDict()
        sdict["stage_number"] = get_attr(STAGE_NUMBER, s)
        sdict["memories"] = get_stage_memories(s, table, context, entries_so_far)
        sdict["overhead_fields"] = get_overhead_fields(s, table)
        sdict["match_format"] = get_stage_match_format(s, table)
        sdict["action_formats"] = get_stage_action_formats(s, table, context)
        entries_so_far += get_attr(SIZE, s)
        stage_list.append(sdict)
    return stage_list

def get_unit_stage_allocation(table, context):
    """
    This function is used for getting the stage allocations for tables that have unit-based structures,
    like algorithmic TCAM and chained LPM.
    """
    stage_list = []

    match_attr = get_attr(MATCH_ATTRIBUTES, table)
    match_type = get_attr(MATCH_TYPE, match_attr)
    entries_so_far = 0
    if match_type == ALGORITHMIC_LPM:
        atcam_table = get_attr(ATCAM_TABLE, match_attr)
        match_attr2 = get_attr(MATCH_ATTRIBUTES, atcam_table)
        units = get_attr(UNITS, match_attr2)
    else:
        units = get_attr(UNITS, match_attr)
    for uidx, unit in enumerate(units):
        stage_tables = get_match_stage_tables(unit)
        for s in stage_tables:
            sdict = OrderedDict()
            sdict["stage_number"] = get_attr(STAGE_NUMBER, s)
            sdict["memories"] = get_stage_memories(s, table, context, entries_so_far, uidx)
            sdict["overhead_fields"] = get_overhead_fields(s, unit)
            sdict["match_format"] = get_stage_match_format(s, unit)
            sdict["action_formats"] = get_stage_action_formats(s, table, context)
            entries_so_far += get_attr(SIZE, s)
            stage_list.append(sdict)

    if match_type == ALGORITHMIC_LPM:
        pre_classifier = get_attr(PRE_CLASSIFIER, match_attr)
        pre_stage_tables = get_match_stage_tables(pre_classifier)
        entries_so_far = 0
        for s in pre_stage_tables:
            sdict = OrderedDict()
            sdict["stage_number"] = get_attr(STAGE_NUMBER, s)
            sdict["memories"] = get_stage_memories(s, pre_classifier, context, entries_so_far)
            sdict["overhead_fields"] = get_overhead_fields(s, pre_classifier)
            sdict["match_format"] = get_stage_match_format(s, pre_classifier)
            sdict["action_formats"] = get_stage_action_formats(s, pre_classifier, context)
            entries_so_far += get_attr(SIZE, s)
            stage_list.append(sdict)

    return stage_list


def produce_match_table_json(table, context):
    tdict = OrderedDict()
    match_attr = get_attr(MATCH_ATTRIBUTES, table)
    match_type = get_attr(MATCH_TYPE, match_attr)

    tdict["name"] = get_attr(NAME, table)
    tdict["gress"] = get_attr(DIRECTION, table)
    tdict["lookup_types"] = []  # Added this just so diff could be easier
    tdict["entries_requested"] = get_attr(SIZE, table)
    tdict["entries_allocated"] = get_entries_allocated(table)
    tdict["lookup_types"], tdict["match_fields"] = get_match_key_fields_json(get_attr(MATCH_KEY_FIELDS, table))
    tdict["action_parameters"] = get_action_parameters_json(table)
    if match_type in [ALGORITHMIC_TCAM, ALGORITHMIC_LPM, CHAINED_LPM]:
        tdict["stage_allocation"] = get_unit_stage_allocation(table, context)
    else:
        tdict["stage_allocation"] = get_stage_allocation(table, context)

    return tdict


def produce_tables_json(context):
    all_tables = []

    tables = get_attr(TABLES, context)
    for t in tables:
        table_type = get_attr(TABLE_TYPE, t)
        if table_type == MATCH:
            all_tables.append(produce_match_table_json(t, context))
    all_tables.sort(key=lambda x: x["name"])
    return all_tables


def produce_mau_json(source, output):
    # Load JSON and make sure versions are correct
    json_file = open(source, 'r')
    try:
        context = json.load(json_file)
    except:
        json_file.close()
        context = {}
        print_error_and_exit("Unable to open JSON file '%s'" % str(source))

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_CONTEXT_JSON_VERSION, schema_version):
        error_msg = "Unsupported context.json schema version %s.\n" % str(schema_version)
        error_msg += "Required minimum version is: %s" % str(MINIMUM_CONTEXT_JSON_VERSION)
        print_error_and_exit(error_msg)

    # Create mau.json dictionary
    mau_json = OrderedDict()
    mau_json["build_date"] = get_attr(BUILD_DATE, context)
    mau_json["program_name"] = get_attr(PROGRAM_NAME, context)
    mau_json["run_id"] = get_attr(RUN_ID, context)
    mau_json["compiler_version"] = get_attr(COMPILER_VERSION, context)
    mau_json["schema_version"] = MAU_SCHEMA_VERSION
    mau_json["tables"] = produce_tables_json(context)

    # Tidy up and validate
    json_file.close()

    mau_schema = MauJSONSchema.get_schema()
    cpath = os.path.join(output, MAU_JSON_FILE)
    with open(cpath, "w") as cfile:
        json.dump(mau_json, cfile, indent=4)
    jsonschema.validate(mau_json, mau_schema)
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
        return_code, err_msg = produce_mau_json(args.source, args.output)
    except:
        print_error_and_exit("Unable to create %s." % MAU_JSON_FILE)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

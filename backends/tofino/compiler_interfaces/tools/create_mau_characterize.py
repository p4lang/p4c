#!/usr/bin/env python3

"""
This script produces a mau.characterize.log from an input mau.json file.
"""

import json
import logging
import math
import os
import sys
from collections import OrderedDict

from .utils import *

if not getattr(sys, 'frozen', False):
    # standalone script
    MYPATH = os.path.dirname(__file__)
    SCHEMA_PATH = os.path.join(MYPATH, "../")
    sys.path.append(SCHEMA_PATH)

from schemas.schema_enum_values import *
from schemas.schema_keys import *

# The minimum mau.json schema version required.
MINIMUM_MAU_JSON_VERSION = "1.0.0"
RESULTS_FILE = "mau.characterize.log"
LOGGER_NAME = "MAU-CHAR-RESULT-LOG"


# ----------------------------------------
# Helpers
# ----------------------------------------


class ReqAlloc(object):
    def __init__(self, requested, allocated):
        self.requested = requested
        self.allocated = allocated

    def __str__(self):
        return "%d / %d (%d)" % (self.requested, self.allocated, self.allocated - self.requested)


class MemPack(object):
    def __init__(self, entries, mem_words, mem_word_width):
        self.entries = entries
        self.mem_words = mem_words
        self.mem_word_width = mem_word_width

    def __str__(self):
        return "%d in %d (%d)" % (
            self.entries,
            self.mem_words,
            self.mem_words * self.mem_word_width,
        )


class PackEff(object):
    def __init__(self, ideal=None, actual=None):
        self.ideal = ideal
        self.actual = actual

    def __str__(self):
        if self.ideal is None or self.actual is None:
            return "- / -"
        else:
            return "%.1f%% / %.1f%%" % (self.ideal, self.actual)


class MatchOverhead(object):
    def __init__(
        self,
        next_table=0,
        action_instr=0,
        action_ptr=0,
        meter_ptr=0,
        stat_ptr=0,
        sel_len=0,
        version=0,
        imm=0,
    ):
        self.next_table = next_table
        self.action_instr = action_instr
        self.action_ptr = action_ptr
        self.meter_ptr = meter_ptr
        self.stat_ptr = stat_ptr
        self.sel_len = sel_len
        self.version = version
        self.imm = imm

    def __str__(self):
        return "%d/%d/%d/%d/%d/%d/%d/%d" % (
            self.next_table,
            self.action_instr,
            self.action_ptr,
            self.meter_ptr,
            self.stat_ptr,
            self.sel_len,
            self.version,
            self.imm,
        )

    def log_it(self, table_name):
        to_ret = "+----------------------------------------------------------------+\n"
        to_ret += "   %s\n" % str(table_name)
        to_ret += "+----------------------------------------------------------------+\n"
        to_ret += "Match Overhead:\n"

        NT_NAME = "--next_table--"
        SEL_LEN_NAME = "--selection_length--"
        VV_NAME = "--version_valid--"
        AI_NAME = "--instruction_address--"
        ADT_NAME = "--action_data_pointer--"
        STAT_NAME = "--statistics_pointer--"
        METER_NAME = "--meter_pointer--"
        IMM_NAME = "--immediate--"

        # These are ordered this way to just to making diffing against existing logs easier.
        ordered_fields = [
            (self.next_table, NT_NAME),
            (self.sel_len, SEL_LEN_NAME),
            (self.version, VV_NAME),
            (self.action_instr, AI_NAME),
            (self.action_ptr, ADT_NAME),
            (self.stat_ptr, STAT_NAME),
            (self.meter_ptr, METER_NAME),
            (self.imm, IMM_NAME),
        ]

        for f, fname in ordered_fields:
            if f > 0:
                plural = ""
                if f > 1:
                    plural = "s"
                to_ret += "  Field %s [%d:0] (%d bit%s)\n" % (fname, f - 1, f, plural)

        to_ret += "\n  Total bits: %d" % self.total()
        return to_ret

    def total(self):
        return (
            self.next_table
            + self.action_instr
            + self.action_ptr
            + self.meter_ptr
            + self.stat_ptr
            + self.sel_len
            + self.version
            + self.imm
        )

    def add(self, other):
        self.next_table += other.next_table
        self.action_instr += other.action_instr
        self.action_ptr += other.action_ptr
        self.meter_ptr += other.meter_ptr
        self.stat_ptr += other.stat_ptr
        self.sel_len += other.sel_len
        self.version += other.version
        self.imm += other.imm


class SramUse(object):
    def __init__(self, match=0, action=0, stats=0, meter=0, tind=0):
        self.match = match
        self.action = action
        self.stats = stats
        self.meter = meter
        self.tind = tind

    def __str__(self):
        return "%d (%d/%d/%d/%d/%d)" % (
            self.total(),
            self.match,
            self.action,
            self.stats,
            self.meter,
            self.tind,
        )

    def uses_match_memory(self):
        return self.match > 0

    def total(self):
        return self.match + self.action + self.stats + self.meter + self.tind

    def add(self, other):
        self.match += other.match
        self.action += other.action
        self.stats += other.stats
        self.meter += other.meter
        self.tind += other.tind


class TableRow(object):
    def __init__(
        self,
        name,
        gress,
        stage,
        lookup,
        mem_type,
        sram_use,
        tcam,
        entries_req_alloc,
        match_bits_req_alloc,
        tcam_overhead,
        sram_overhead,
        match_overhead,
        imm_req_alloc,
        tcam_bits_req_alloc,
        sram_bits_req_alloc,
        p4_action_bits,
        action_bits_req_alloc,
        ideal_match_entries,
        actual_match_entries,
        tcam_eff,
        sram_match_eff,
        sram_action_eff,
    ):
        self.name = name
        self.gress = gress
        self.stage = stage
        self.lookup = lookup
        self.mem_type = mem_type

        self.sram_use = sram_use
        self.tcam = tcam
        self.entries_req_alloc = entries_req_alloc
        self.match_bits_req_alloc = match_bits_req_alloc
        self.tcam_overhead = tcam_overhead

        self.sram_overhead = sram_overhead
        self.match_overhead = match_overhead
        self.imm_req_alloc = imm_req_alloc
        self.tcam_bits_req_alloc = tcam_bits_req_alloc
        self.sram_bits_req_alloc = sram_bits_req_alloc

        self.p4_action_bits = p4_action_bits
        self.action_bits_req_alloc = action_bits_req_alloc
        self.ideal_match_entries = ideal_match_entries
        self.actual_match_entries = actual_match_entries
        self.tcam_eff = tcam_eff

        self.sram_match_eff = sram_match_eff
        self.sram_action_eff = sram_action_eff

    def get_row(self):
        return [
            self.name,
            self.gress,
            self.stage,
            self.lookup,
            self.mem_type,
            self.sram_use,
            self.tcam,
            self.entries_req_alloc,
            self.match_bits_req_alloc,
            self.tcam_overhead,
            self.sram_overhead,
            self.match_overhead,
            self.imm_req_alloc,
            self.tcam_bits_req_alloc,
            self.sram_bits_req_alloc,
            self.p4_action_bits,
            self.action_bits_req_alloc,
            self.ideal_match_entries,
            self.actual_match_entries,
            self.tcam_eff,
            self.sram_match_eff,
            self.sram_action_eff,
        ]


# ----------------------------------------
#  Produce log file
# ----------------------------------------


def _parse_mau_json(context):

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_MAU_JSON_VERSION, schema_version):
        error_msg = "Unsupported context.json schema version %s.\n" % str(schema_version)
        error_msg += "Required minimum version is: %s" % str(MINIMUM_MAU_JSON_VERSION)
        print_error_and_exit(error_msg)

    tables = get_attr(TABLES, context)

    # (table name, gress, stage number) -> TableRow
    table_info = OrderedDict()
    sram_summary = OrderedDict()
    # (table name, gress, stage number) -> cnt
    table_name_saw = OrderedDict()
    # (table name, stage) -> overhead structure
    all_overhead_structures = OrderedDict()
    all_match_and_action_formats = OrderedDict()

    for t in tables:
        table_name = get_attr(NAME, t)
        gress = get_attr(GRESS, t)
        lookup_types = get_attr(LOOKUP_TYPES, t)
        ltypes = []
        for lt in lookup_types:
            ltypes.append(str(lt))
        ltypes.sort()

        # entries_requested = get_attr(ENTRIES_REQUESTED, t)
        # entries_allocated = get_attr(ENTRIES_ALLOCATED, t)

        match_fields = get_attr(MATCH_FIELDS, t)
        total_match_bits = 0
        match_fields_cache = {}
        for m in match_fields:
            mname = get_attr(NAME, m)
            bit_width = get_attr(BIT_WIDTH, m)
            total_match_bits += bit_width
            if mname in match_fields_cache:
                match_fields_cache[mname] += bit_width
            else:
                match_fields_cache[mname] = bit_width

        action_parameters = get_attr(ACTION_PARAMETERS, t)
        max_p4_action_parameters = 0
        for ap in action_parameters:
            action_name = get_attr(ACTION_NAME, ap)
            local_p4_action_parameters = 0
            parameters = get_attr(PARAMETERS, ap)
            for p in parameters:
                bit_width = get_attr(BIT_WIDTH, p)
                local_p4_action_parameters += bit_width
            max_p4_action_parameters = max(local_p4_action_parameters, max_p4_action_parameters)

        stage_allocation = get_attr(STAGE_ALLOCATION, t)
        for salloc in stage_allocation:
            stage_number = get_attr(STAGE_NUMBER, salloc)

            overhead_fields = get_attr(OVERHEAD_FIELDS, salloc)
            tcam_ver_bit_width = 0
            match_overhead_structure = MatchOverhead()
            oh_imm_bit_width = 0
            for o in overhead_fields:
                name = get_attr(NAME, o)
                bit_width = get_attr(BIT_WIDTH, o)

                if name == OH_VERSION:
                    tcam_ver_bit_width = bit_width
                elif name == OH_VERSION_VALID:
                    vv_bit_width = bit_width
                    match_overhead_structure.add(MatchOverhead(version=bit_width))
                elif name == OH_INSTR:
                    match_overhead_structure.add(MatchOverhead(action_instr=bit_width))
                elif name == OH_ADT_PTR:
                    match_overhead_structure.add(MatchOverhead(action_ptr=bit_width))
                elif name in [OH_METER_PTR, OH_SEL_PTR, OH_STFUL_PTR]:
                    match_overhead_structure.add(MatchOverhead(meter_ptr=bit_width))
                elif name == OH_STATS_PTR:
                    match_overhead_structure.add(MatchOverhead(stat_ptr=bit_width))
                elif name == OH_NEXT_TABLE:
                    match_overhead_structure.add(MatchOverhead(next_table=bit_width))
                elif name in [OH_SELECTION_LENGTH, OH_SELECTION_LENGTH_SHIFT]:
                    match_overhead_structure.add(MatchOverhead(sel_len=bit_width))
                elif name == OH_IMMEDIATE:
                    match_overhead_structure.add(MatchOverhead(imm=bit_width))
                    oh_imm_bit_width = bit_width

            all_overhead_structures[(table_name, stage_number)] = match_overhead_structure

            match_format = get_attr(MATCH_FORMAT, salloc)
            entries = get_attr(ENTRIES, match_format)
            pe_match_bits = 0
            for e in entries:
                fields = get_attr(FIELDS, e)
                for f in fields:
                    fname = get_attr(NAME, f)
                    bit_width = get_attr(BIT_WIDTH, f)
                    if fname in match_fields_cache:
                        pe_match_bits += bit_width
                break  # only want first entry

            memories = get_attr(MEMORIES, salloc)
            sram_use = SramUse()
            tcam_use = 0
            mem_types = []
            entries_req_alloc = ReqAlloc(0, 0)
            sram_bits_req_alloc = ReqAlloc(0, 0)
            allocated_entry_width = 0
            imm_bits_req_alloc = ReqAlloc(0, 0)
            tcam_bits_req_alloc = ReqAlloc(0, 0)
            ideal_match_entries = MemPack(0, 0, 0)
            actual_match_entries = MemPack(0, 0, 0)
            actual_action_entries = MemPack(0, 0, 0)
            action_bits_req_alloc = ReqAlloc(0, 0)
            tcam_eff = PackEff()
            sram_eff = PackEff()
            act_eff = PackEff()

            for m in memories:
                memory_type = get_attr(MEMORY_TYPE, m)
                table_word_width = get_attr(TABLE_WORD_WIDTH, m)
                memory_word_width = get_attr(MEMORY_WORD_WIDTH, m)
                entries_per_table_word = get_attr(ENTRIES_PER_TABLE_WORD, m)
                table_type = get_attr(TABLE_TYPE, m)
                num_memories = get_attr(NUM_MEMORIES, m)

                if table_type == MATCH:
                    if memory_type not in mem_types:
                        mem_types.append(memory_type)

                if memory_type == SRAM:
                    if table_type == ACTION:
                        sram_use.add(SramUse(action=num_memories))
                    elif table_type == MATCH:
                        sram_use.add(SramUse(match=num_memories))

                    elif table_type in [METER, SELECTION, STATEFUL]:
                        sram_use.add(SramUse(meter=num_memories))

                    elif table_type == STATISTICS:
                        sram_use.add(SramUse(stats=num_memories))

                    elif table_type == TIND:
                        sram_use.add(SramUse(tind=num_memories))

                elif memory_type == TCAM:
                    tcam_use += num_memories

                # optional attributes
                entry_bit_width_requested = get_optional_attr(ENTRY_BIT_WIDTH_REQUESTED, m)
                entry_bit_width_allocated = get_optional_attr(ENTRY_BIT_WIDTH_ALLOCATED, m)
                if entry_bit_width_requested is not None and entry_bit_width_allocated is not None:
                    if table_type == TIND and memory_type == SRAM:
                        sram_bits_req_alloc = ReqAlloc(
                            entry_bit_width_requested, entry_bit_width_allocated
                        )
                    elif table_type == MATCH and memory_type == TCAM:
                        allocated_entry_width = total_match_bits  # This seems confusing
                        tcam_bits_req_alloc = ReqAlloc(
                            entry_bit_width_requested, entry_bit_width_allocated
                        )

                    elif table_type == MATCH and memory_type == SRAM:
                        sram_bits_req_alloc = ReqAlloc(
                            entry_bit_width_requested, entry_bit_width_allocated
                        )
                        allocated_entry_width = max(
                            0, entry_bit_width_allocated - match_overhead_structure.total()
                        )
                    elif table_type == ACTION and memory_type == SRAM:
                        action_bits_req_alloc = ReqAlloc(
                            entry_bit_width_requested, entry_bit_width_allocated
                        )

                # Need for computing memory packing efficiencies
                ideal_overhead_imm_bit_width = 0
                for m2 in memories:
                    table_type_2 = get_attr(TABLE_TYPE, m2)
                    if table_type_2 == MATCH:
                        imm_bit_width_in_overhead_requested_2 = get_optional_attr(
                            IMM_BIT_WIDTH_IN_OVERHEAD_REQUESTED, m2
                        )
                        if imm_bit_width_in_overhead_requested_2 is not None:
                            ideal_overhead_imm_bit_width = imm_bit_width_in_overhead_requested_2
                        break

                ideal_entries_per_table_word = get_optional_attr(IDEAL_ENTRIES_PER_TABLE_WORD, m)
                ideal_table_word_bit_width = get_optional_attr(IDEAL_TABLE_WORD_BIT_WIDTH, m)
                if (
                    ideal_entries_per_table_word is not None
                    and ideal_table_word_bit_width is not None
                ):
                    if table_type == MATCH and memory_word_width != 0:
                        ideal_match_entries = MemPack(
                            ideal_entries_per_table_word,
                            ideal_table_word_bit_width // memory_word_width,
                            memory_word_width,
                        )
                        actual_match_entries = MemPack(
                            entries_per_table_word,
                            table_word_width // memory_word_width,
                            memory_word_width,
                        )

                        if ideal_table_word_bit_width != 0 and entry_bit_width_allocated != 0:
                            if memory_type == TCAM:
                                ideal_tcam_eff = (
                                    100.0 * total_match_bits / float(ideal_table_word_bit_width)
                                )
                                actual_tcam_eff = (
                                    100.0 * total_match_bits / float(entry_bit_width_allocated)
                                )
                                tcam_eff = PackEff(ideal_tcam_eff, actual_tcam_eff)
                            elif memory_type == SRAM:
                                local_match_bits = total_match_bits + ideal_overhead_imm_bit_width
                                ideal_sram_eff = (
                                    100.0
                                    * ideal_entries_per_table_word
                                    * local_match_bits
                                    / float(ideal_table_word_bit_width)
                                )

                                all_bits_used = entries_per_table_word * (
                                    pe_match_bits + oh_imm_bit_width
                                )
                                actual_sram_eff = 100.0 * all_bits_used / float(table_word_width)
                                sram_eff = PackEff(ideal_sram_eff, actual_sram_eff)

                if table_type == TIND and memory_word_width != 0:
                    if ideal_overhead_imm_bit_width > 0:
                        max_tind_entries = 128 // ideal_overhead_imm_bit_width
                        ideal_sram_efficiency = (
                            100.0 * max_tind_entries * ideal_overhead_imm_bit_width / 128.0
                        )
                        if entry_bit_width_allocated > 0:
                            actual_sram_efficiency = (
                                100.0
                                * ideal_overhead_imm_bit_width
                                / float(entry_bit_width_allocated)
                            )
                        else:
                            actual_sram_efficiency = 0
                        sram_eff = PackEff(ideal_sram_efficiency, actual_sram_efficiency)

                elif table_type == ACTION and memory_word_width != 0:
                    ideal_action_bits_per_entry = max(
                        0, max_p4_action_parameters - oh_imm_bit_width
                    )
                    leftover_action = max(0, max_p4_action_parameters - oh_imm_bit_width)
                    actual_action_entries = MemPack(
                        entries_per_table_word,
                        table_word_width // memory_word_width,
                        memory_word_width,
                    )

                    if leftover_action > 128:
                        ideal_action_eff = leftover_action / float(entry_bit_width_requested)
                    else:
                        if leftover_action > 0:
                            ideal_action_packing = 128 // leftover_action
                        else:
                            ideal_action_packing = 0
                        if entry_bit_width_requested > 0:
                            ideal_action_eff = (
                                100.0 * leftover_action * ideal_action_packing / 128.0
                            )
                        else:
                            ideal_action_eff = 0

                    # ideal_action_efficiency = 100.0 * ideal_entries_per_table_word * leftover_action / float(ideal_table_word_bit_width)
                    if entry_bit_width_allocated > 0:
                        actual_action_efficiency = (
                            100.0 * leftover_action / float(entry_bit_width_allocated)
                        )
                    else:
                        actual_action_efficiency = 0
                    act_eff = PackEff(ideal_action_eff, actual_action_efficiency)

                entries_requested = get_optional_attr(ENTRIES_REQUESTED, m)
                entries_allocated = get_optional_attr(ENTRIES_ALLOCATED, m)
                if entries_requested is not None and entries_allocated is not None:
                    entries_req_alloc = ReqAlloc(entries_requested, entries_allocated)

                imm_bit_width_in_overhead_requested = get_optional_attr(
                    IMM_BIT_WIDTH_IN_OVERHEAD_REQUESTED, m
                )
                imm_bit_width_in_overhead_allocated = get_optional_attr(
                    IMM_BIT_WIDTH_IN_OVERHEAD_ALLOCATED, m
                )
                if (
                    imm_bit_width_in_overhead_requested is not None
                    and imm_bit_width_in_overhead_allocated is not None
                ):
                    imm_bits_req_alloc = ReqAlloc(
                        imm_bit_width_in_overhead_requested, imm_bit_width_in_overhead_allocated
                    )

            mem_types.sort()

            match_bits_req_alloc = ReqAlloc(total_match_bits, allocated_entry_width)

            action_formats = get_attr(ACTION_FORMATS, salloc)
            all_match_and_action_formats[(table_name, stage_number)] = (
                match_format,
                actual_match_entries,
                action_formats,
                actual_action_entries,
            )

            sn = stage_number
            if sn == -1:
                sn = "parser"

            trow = TableRow(
                table_name,
                gress,
                sn,
                list2str(ltypes),
                list2str(mem_types),
                str(sram_use),
                tcam_use,
                obj2str(entries_req_alloc),
                obj2str(match_bits_req_alloc),
                tcam_ver_bit_width,
                match_overhead_structure.total(),
                obj2str(match_overhead_structure),
                obj2str(imm_bits_req_alloc),
                obj2str(tcam_bits_req_alloc),
                obj2str(sram_bits_req_alloc),
                max_p4_action_parameters,
                obj2str(action_bits_req_alloc),
                obj2str(ideal_match_entries),
                obj2str(actual_match_entries),
                obj2str(tcam_eff),
                obj2str(sram_eff),
                obj2str(act_eff),
            )

            # Handle things like Algorithmic TCAM or Chained LPM, which can appear in the same stage multiple times.
            adj_table_name = table_name
            if (table_name, gress, stage_number) in table_name_saw:
                adj_table_name = "%s_%d" % (
                    table_name,
                    table_name_saw[(table_name, gress, stage_number)],
                )
                trow.name = adj_table_name
            table_info[(adj_table_name, gress, stage_number)] = trow
            sram_summary[(adj_table_name, gress, stage_number)] = sram_use

            if (table_name, gress, stage_number) not in table_name_saw:
                table_name_saw[(table_name, gress, stage_number)] = 0
            table_name_saw[(table_name, gress, stage_number)] += 1

    return table_info, sram_summary, all_overhead_structures, all_match_and_action_formats


def log_overhead_structures(all_overhead_structures):
    log = logging.getLogger(LOGGER_NAME)
    keys = list(all_overhead_structures.keys())
    tbls_saw = set()
    keys.sort(key=lambda x: (x[0], x[1]))  # Sort by name then stage number

    log.info("\n\n+----------------------------------------------------------------+")
    log.info("    OVERHEAD STRUCTURES")
    log.info("+----------------------------------------------------------------+\n")

    for name, stage in keys:
        if name in tbls_saw:
            continue
        mo = all_overhead_structures[(name, stage)]
        log.info(mo.log_it(name))


def log_pack_format(pack_format_json, actual_entries, mem_type):
    log = logging.getLogger(LOGGER_NAME)

    if actual_entries.mem_words == 0 or actual_entries.mem_word_width == 0:
        log.info("No %s entries required." % mem_type)
        return

    entries = get_attr(ENTRIES, pack_format_json)
    if len(entries) > 0:
        log.info("Pack Format:")
        log.info(
            "  table_word_width: %d" % (actual_entries.mem_words * actual_entries.mem_word_width)
        )
        log.info("  memory_word_width: %d" % actual_entries.mem_word_width)
        log.info("  entries_per_table_word: %d" % len(entries))
        log.info("  number_memory_units_per_table_word: %d" % actual_entries.mem_words)
        log.info("  entry_list: [")

    for e in entries:
        entry_number = get_attr(ENTRY_NUMBER, e)
        log.info("    entry_number : %d" % entry_number)
        log.info("    field_list : [")
        fields = get_attr(FIELDS, e)

        max_name_len = 1
        all_fields = []
        for f in fields:
            name = get_attr(NAME, f)
            bit_width = get_attr(BIT_WIDTH, f)
            start_bit = get_attr(START_BIT, f)
            memory_start_bit = get_attr(MEMORY_START_BIT, f)

            name_with_bits = "%s [%d:%d]" % (name, start_bit + bit_width - 1, start_bit)
            max_name_len = max(max_name_len, len(name_with_bits))

            in_bits = "[%d:%d]" % (memory_start_bit + bit_width - 1, memory_start_bit)
            all_fields.append((name_with_bits, in_bits))

        for fname, in_bits in all_fields:
            n_pad = "%s" % fname
            n_pad = n_pad.ljust(max_name_len + 2)
            log.info("       Field %s : in bits %s" % (n_pad, in_bits))

        log.info("    ]")

    if len(entries) > 0:
        log.info("  ]")


def log_action_formats(stage, action_formats_json, actual_action_entries):
    log = logging.getLogger(LOGGER_NAME)

    for action_format_dict in action_formats_json:
        name = get_attr(NAME, action_format_dict)
        action_format = get_attr(ACTION_FORMAT, action_format_dict)

        log.info("\nAction %s" % name)
        log.info("---------------------------")
        log_pack_format(action_format, actual_action_entries, "action")

        log.info("\nAction data bus usage:")
        log.info("Stage: %d" % stage)

        hdrs = [["Parameter", "Action data slot size", "Action data slot number"]]
        data = []
        slot_cnt = {}

        parameter_map = get_attr(PARAMETER_MAP, action_format_dict)
        for pmap in parameter_map:
            name = get_attr(NAME, pmap)
            bit_width = get_attr(BIT_WIDTH, pmap)
            start_bit = get_attr(START_BIT, pmap)
            location = get_attr(LOCATION, pmap)
            action_data_slot_size = get_attr(ACTION_DATA_SLOT_SIZE, pmap)
            action_slot_number = get_attr(ACTION_SLOT_NUMBER, pmap)

            data.append(
                [
                    "%s[%d:%d]" % (name, start_bit + bit_width - 1, start_bit),
                    action_data_slot_size,
                    action_slot_number,
                ]
            )

            if action_data_slot_size not in slot_cnt:
                slot_cnt[action_data_slot_size] = 0
            slot_cnt[action_data_slot_size] += 1

        hdrs2 = [["Action data slot size", "Number in use"]]
        data2 = []
        cnt_keys = list(slot_cnt.keys())
        cnt_keys.sort()
        for k in cnt_keys:
            data2.append([k, slot_cnt[k]])

        if len(data) == 0 and actual_action_entries.mem_word_width != 0:
            log.info(
                "No action parameter mapping provided.  Action data bus usage will not be displayed."
            )
        elif len(data) == 0:
            log.info("No action data bus usage.")
        else:
            log.info(print_table(hdrs, data))
            log.info(print_table(hdrs2, data2))


def log_match_and_action_formats(all_match_and_action_formats):
    log = logging.getLogger(LOGGER_NAME)
    keys = list(all_match_and_action_formats.keys())
    keys.sort(key=lambda x: (x[0], x[1]))  # Sort by name then stage number

    tbl_to_info = OrderedDict()
    for table_name, stage in keys:
        if table_name not in tbl_to_info:
            tbl_to_info[table_name] = OrderedDict()
        match_format_json, actual_match_entries, action_formats_json, actual_action_entries = (
            all_match_and_action_formats[(table_name, stage)]
        )
        tbl_to_info[table_name][stage] = (
            match_format_json,
            actual_match_entries,
            action_formats_json,
            actual_action_entries,
        )

    log.info("\n\n\n\n\n+----------------------------------------------------------------+")
    log.info("    MATCH AND ACTION FORMATS")
    log.info("+----------------------------------------------------------------+\n")

    for table_name in tbl_to_info:

        log.info("+----------------------------------------------------------------+")
        log.info("  %s" % table_name)
        log.info("+----------------------------------------------------------------+")

        all_match_formats = []
        all_action_formats = []
        for stage in tbl_to_info[table_name]:
            match_format_json, actual_match_entries, action_formats_json, actual_action_entries = (
                tbl_to_info[table_name][stage]
            )
            all_match_formats.append((stage, match_format_json, actual_match_entries))
            all_action_formats.append((stage, action_formats_json, actual_action_entries))

        log.info("+------------------------------------------------+")
        log.info("   Match format for %s" % table_name)
        log.info("+------------------------------------------------+")
        for stage, match_format_json, actual_match_entries in all_match_formats:
            log.info("Stage %d:" % stage)
            log_pack_format(match_format_json, actual_match_entries, "match")

        for stage, action_formats_json, actual_action_entries in all_action_formats:
            log_action_formats(stage, action_formats_json, actual_action_entries)


def produce_mau_characterize(source, output):
    # Setup logging
    log_file = os.path.join(output, RESULTS_FILE)
    logging_file_handle = logging.FileHandler(log_file, mode='w')
    log = logging.getLogger(LOGGER_NAME)
    log.setLevel(logging.INFO)
    log.propagate = 0
    logging_file_handle.setLevel(logging.INFO)
    log.addHandler(logging_file_handle)

    # Load JSON and make sure versions are correct
    json_file = open(source, 'r')
    try:
        context = json.load(json_file)
    except:
        json_file.close()
        print_error_and_exit("Unable to open JSON file '%s'" % str(source))

    compiler_version = get_attr(COMPILER_VERSION, context)
    build_date = get_attr(BUILD_DATE, context)
    run_id = get_attr(RUN_ID, context)

    box = "+---------------------------------------------------------------------+"
    log.info(box)
    for s in [
        "Log file: %s" % RESULTS_FILE,
        "Compiler version: %s" % str(compiler_version),
        "Created on: %s" % str(build_date),
        "Run ID: %s" % str(run_id),
    ]:
        line = "|  %s" % s
        while len(line) < (len(box) - 1):
            line += " "
        line += "|"
        log.info(line)
    log.info("%s\n" % box)

    # Populate table summary information
    table_info, sram_summary, all_overhead_structures, all_match_and_action_formats = (
        _parse_mau_json(context)
    )

    # Output summary table in log file

    hdrs = [
        [
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16",
            "17",
            "18",
            "19",
            "20",
            "21",
            "22",
        ],
        [
            "Table",
            "Dir",
            "Stage",
            "P4",
            "Mem",
            "Total",
            "Total",
            "Table",
            "Match",
            "TCAM",
            "SRAM",
            "Match",
            "Imm.",
            "TCAM",
            "SRAM",
            "P4",
            "Action",
            "Ideal",
            "Actual",
            "TCAM",
            "SRAM",
            "SRAM",
        ],
        [
            "Name",
            "",
            "",
            "Lookup",
            "Type",
            "SRAMs",
            "TCAMs",
            "Entries",
            "Bits",
            "Over-",
            "Over-",
            "Overhead",
            "Action",
            "Bits",
            "Bits",
            "Action",
            "Bits",
            "Match",
            "Match",
            "Match",
            "Match",
            "Action",
        ],
        [
            "",
            "",
            "",
            "Type(s)",
            "",
            "TOT(M/A/S/MT/I)",
            "",
            "Requested",
            "Per",
            "head",
            "head",
            "Structure",
            "Data",
            "Per",
            "Per",
            "Bits",
            "Per",
            "Entries-",
            "Entries-",
            "Packing",
            "Packing",
            "Packing",
        ],
        [
            "",
            "",
            "",
            "",
            "",
            "(legend",
            "",
            "/",
            "Entry",
            "Bits",
            "Bits",
            "NT/AI/AD/M/S/SL/V/I",
            "in",
            "Entry",
            "Entry",
            "",
            "Entry",
            "Number",
            "Number",
            "Eff.",
            "Eff.",
            "Eff.",
        ],
        [
            "",
            "",
            "",
            "",
            "",
            "below)",
            "",
            "Allocated",
            "R/A(diff)",
            "Per",
            "Per",
            "(legend",
            "Overhead",
            "R/A(diff)",
            "R/A(diff)",
            "",
            "R/A(diff)",
            "Per",
            "Per",
            "Ideal/",
            "Ideal/",
            "Ideal/",
        ],
        [
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "(diff)",
            "",
            "Entry",
            "Entry",
            "below)",
            "R/A(diff)",
            "",
            "",
            "",
            "",
            "Memory",
            "Memory",
            "Actual",
            "Actual",
            "Actual",
        ],
        [
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "ver/vld",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "Units",
            "Units",
            "",
            "",
            "",
        ],
        [
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "(bits)",
            "(bits)",
            "",
            "",
            "",
        ],
    ]

    data = []

    table_info_keys = list(table_info.keys())

    # Sort by stage, gress, table name
    def sort_gress(g):
        if g == INGRESS:
            return 0
        return 1

    table_info_keys.sort(key=lambda x: (x[2], sort_gress(x[1]), x[0]))

    blank = [""] * len(hdrs[0])

    last_stage = None
    current_tcams = 0
    current_srams = SramUse()
    stages_output = set()
    all_srams = SramUse()
    ig_srams = SramUse()
    eg_srams = SramUse()
    all_tcams = 0
    ig_tcams = 0
    eg_tcams = 0

    for n, g, s in table_info_keys:
        if last_stage is not None and s != last_stage:
            summary = ["-"] * len(hdrs[0])
            summary[0] = "stage %s totals" % str(last_stage)
            summary[5] = obj2str(current_srams)
            summary[6] = current_tcams
            if last_stage != -1:
                data.append(summary)
                stages_output.add(last_stage)
            data.append(blank)
            current_tcams = 0
            current_srams = SramUse()
        row = table_info[(n, g, s)]
        sram_use = sram_summary[(n, g, s)]
        data.append(row.get_row())
        last_stage = s
        current_srams.add(sram_use)
        current_tcams += row.tcam
        all_srams.add(sram_use)
        if g == INGRESS:
            ig_srams.add(sram_use)
            ig_tcams += row.tcam
        elif g == EGRESS:
            eg_srams.add(sram_use)
            eg_tcams += row.tcam
        all_tcams += row.tcam

    if last_stage is not None and last_stage not in stages_output:
        summary = ["-"] * len(hdrs[0])
        summary[0] = "stage %s totals" % str(last_stage)
        summary[5] = obj2str(current_srams)
        summary[6] = current_tcams
        data.append(summary)

    overall_ig = ["-"] * len(hdrs[0])
    overall_ig[0] = "overall totals (ingress)"
    overall_ig[5] = obj2str(ig_srams)
    overall_ig[6] = ig_tcams
    data.append(blank)
    data.append(overall_ig)

    overall_eg = ["-"] * len(hdrs[0])
    overall_eg[0] = "overall totals (egress)"
    overall_eg[5] = obj2str(eg_srams)
    overall_eg[6] = eg_tcams
    data.append(blank)
    data.append(overall_eg)

    overall = ["-"] * len(hdrs[0])
    overall[0] = "overall totals"
    overall[5] = obj2str(all_srams)
    overall[6] = all_tcams
    data.append(blank)
    data.append(overall)

    log.info("Match+Action Resource Usage")
    log.info(print_table(hdrs, data))

    log.info("Total SRAMs Legend:")
    log.info("TOT (M/A/S/MT/I)")
    log.info("TOT = Total")
    log.info("M = Match")
    log.info("A = Action")
    log.info("S = Statistics")
    log.info("MT = Meter / Stateful / Selection")
    log.info("I = Ternary Indirection\n")

    log.info("Match Overhead Structure Legend:")
    log.info("NT/AI/AD/M/S/SL/V/I")
    log.info("NT = Next Table Pointer")
    log.info("AI = Action Instruction Pointer")
    log.info("AD = Action Data Pointer")
    log.info("M = Meter/Selection/Stateful Pointer")
    log.info("S = Statistics Pointer")
    log.info("SL = Selection Length")
    log.info("V = Entry Version")
    log.info("I = Immediate Action Data\n\n\n")

    log_overhead_structures(all_overhead_structures)
    log_match_and_action_formats(all_match_and_action_formats)

    # Tidy up
    json_file.close()

    return SUCCESS, "ok"


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument(
        'source', metavar='source', type=str, help='The input mau.json source file to use.'
    )
    parser.add_argument(
        '--output',
        '-o',
        type=str,
        action="store",
        default=".",
        help="The output directory to output %s." % RESULTS_FILE,
    )
    args = parser.parse_args()

    try:
        if not os.path.exists(args.output):
            os.mkdir(args.output)
    except:
        print_error_and_exit("Unable to create directory %s." % str(args.output))

    return_code = SUCCESS
    err_msg = ""
    try:
        return_code, err_msg = produce_mau_characterize(args.source, args.output)
    except:
        raise
        print_error_and_exit("Unable to create %s." % RESULTS_FILE)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

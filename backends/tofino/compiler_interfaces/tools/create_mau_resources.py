#!/usr/bin/env python3

"""
This script produces a mau.resources.log from an input resources.json file.
"""

import json
import logging
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

# The minimum resources.json schema version required.
MINIMUM_RESOURCES_JSON_REQUIRED = "1.1.0"
RESULTS_FILE = "mau.resources.log"
LOGGER_NAME = "MAU-RESOURCE-RESULT-LOG"


class Totals(object):
    def __init__(
        self,
        exm_xbar=0,
        tern_xbar=0,
        hash_bits=0,
        hash_dist_units=0,
        gateway=0,
        sram=0,
        map_ram=0,
        tcam=0,
        vliw=0,
        meter_alu=0,
        stats_alu=0,
        stash=0,
        exm_search_bus=0,
        exm_result_bus=0,
        tind_result_bus=0,
        action_data_bytes=0,
        action_slots_8=0,
        action_slots_16=0,
        action_slots_32=0,
        logical_table_id=0,
    ):
        self.exm_xbar = exm_xbar
        self.tern_xbar = tern_xbar
        self.hash_bits = hash_bits
        self.hash_dist_units = hash_dist_units
        self.gateway = gateway
        self.sram = sram
        self.map_ram = map_ram
        self.tcam = tcam
        self.vliw = vliw
        self.meter_alu = meter_alu
        self.stats_alu = stats_alu
        self.stash = stash
        self.exm_search_bus = exm_search_bus
        self.exm_result_bus = exm_result_bus
        self.tind_result_bus = tind_result_bus
        self.action_data_bytes = action_data_bytes
        self.action_slots_8 = action_slots_8
        self.action_slots_16 = action_slots_16
        self.action_slots_32 = action_slots_32
        self.logical_table_id = logical_table_id

    def add(self, other):
        self.exm_xbar += other.exm_xbar
        self.tern_xbar += other.tern_xbar
        self.hash_bits += other.hash_bits
        self.hash_dist_units += other.hash_dist_units
        self.gateway += other.gateway
        self.sram += other.sram
        self.map_ram += other.map_ram
        self.tcam += other.tcam
        self.vliw += other.vliw
        self.meter_alu += other.meter_alu
        self.stats_alu += other.stats_alu
        self.stash += other.stash
        self.exm_search_bus += other.exm_search_bus
        self.exm_result_bus += other.exm_result_bus
        self.tind_result_bus += other.tind_result_bus
        self.action_data_bytes += other.action_data_bytes
        self.action_slots_8 += other.action_slots_8
        self.action_slots_16 += other.action_slots_16
        self.action_slots_32 += other.action_slots_32
        self.logical_table_id += other.logical_table_id

    def get_row(self, tot=None, tables=False):
        if tot is not None:
            row = self.get_row()
            row_tot = tot.get_row()
            assert len(row) == len(row_tot)
            new_row = []
            for idx in range(0, len(row)):
                if row_tot[idx] != 0:
                    util = 100.0 * float(row[idx] / float(row_tot[idx]))
                else:
                    util = 0
                new_row.append("%.2f%%" % util)
            return new_row

        if tables:
            return [
                self.exm_xbar + self.tern_xbar,
                self.hash_bits,
                self.gateway,
                self.sram,
                self.tcam,
                self.map_ram,
                self.action_data_bytes,
                self.vliw,
                self.exm_search_bus,
                self.exm_result_bus,
                self.tind_result_bus,
            ]

        return [
            self.exm_xbar,
            self.tern_xbar,
            self.hash_bits,
            self.hash_dist_units,
            self.gateway,
            self.sram,
            self.map_ram,
            self.tcam,
            self.vliw,
            self.meter_alu,
            self.stats_alu,
            self.stash,
            self.exm_search_bus,
            self.exm_result_bus,
            self.tind_result_bus,
            self.action_data_bytes,
            self.action_slots_8,
            self.action_slots_16,
            self.action_slots_32,
            self.logical_table_id,
        ]


# ----------------------------------------
#  Produce log file
# ----------------------------------------


def _parse_context_json(context):
    # If we ever want to sort the 'tables' table by gress, we can parse context.json to determine gress
    # assignment of each table.

    ingress_names = set()

    # schema_version = get_attr(SCHEMA_VERSION, context)
    # if schema_version not in SUPPORTED_CONTEXT_JSON_VERSIONS:
    #     error_msg = "Unsupported resources.json schema version %s.\n" % str(schema_version)
    #     error_msg += "Supported versions are: %s" % str(SUPPORTED_CONTEXT_JSON_VERSIONS)
    #     print_error_and_exit(error_msg)
    #
    # tables = get_attr(TABLES, context)
    # for t in tables:
    #     name = get_attr(NAME, t)
    #     gress = get_attr(DIRECTION, t)
    #     if gress == INGRESS:
    #         ingress_names.add(name)

    return ingress_names


def _parse_resources_json(context):

    stage_totals = OrderedDict()  # stage number to Totals
    stage_available = OrderedDict()  # stage number to Totals (available)
    table_name_stage = OrderedDict()

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_RESOURCES_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported resources.json schema version %s.\n" % str(schema_version)
        error_msg += "Required minimum version is: %s" % str(MINIMUM_RESOURCES_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    resources = get_attr(RESOURCES, context)
    parser = get_attr(PARSER, resources)
    parsers = get_attr(PARSERS, parser)
    for prsr in parsers:
        if has_attr(PHASE0, prsr):
            phase0 = get_attr(PHASE0, prsr)
            used_by = get_attr(USED_BY, phase0)
            if (used_by, -1) not in table_name_stage:
                table_name_stage[(used_by, -1)] = Totals()

    mau = get_attr(MAU, resources)

    nStages = get_attr(NSTAGES, mau)
    for num in range(0, nStages):
        stage_totals[num] = Totals()
        stage_available[num] = Totals()

    mau_stages = get_attr(MAU_STAGES, mau)
    for mau_stage in mau_stages:
        stage_number = get_attr(STAGE_NUMBER, mau_stage)

        stage_tot = stage_totals[stage_number]
        stage_avail = stage_available[stage_number]

        # ------------------------------------------------------------------------------
        xbar_bytes = get_attr(XBAR_BYTES, mau_stage)
        exact_size = get_attr(EXACT_SIZE, xbar_bytes)
        ternary_size = get_attr(TERNARY_SIZE, xbar_bytes)
        stage_avail.add(Totals(exm_xbar=exact_size, tern_xbar=ternary_size))
        bytes = get_attr(BYTES, xbar_bytes)
        type_to_cnt = {EXACT: 0, TERNARY: 0}
        for b in bytes:
            byte_type = get_attr(BYTE_TYPE, b)
            type_to_cnt[byte_type] += 1
            usages = get_attr(USAGES, b)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                if byte_type == EXACT:
                    table_name_stage[(used_by, stage_number)].add(Totals(exm_xbar=1))
                else:
                    table_name_stage[(used_by, stage_number)].add(Totals(tern_xbar=1))

        stage_tot.add(Totals(exm_xbar=type_to_cnt[EXACT], tern_xbar=type_to_cnt[TERNARY]))

        # ------------------------------------------------------------------------------
        hash_bits = get_attr(HASH_BITS, mau_stage)
        nBits = get_attr(NBITS, hash_bits)
        nFunctions = get_attr(NFUNCTIONS, hash_bits)
        stage_avail.add(Totals(hash_bits=nBits * nFunctions))
        bits = get_attr(BITS, hash_bits)
        cnt = 0
        for b in bits:
            used_by = get_attr(USED_BY_TABLE, b)
            if (used_by, stage_number) not in table_name_stage:
                table_name_stage[(used_by, stage_number)] = Totals()
            table_name_stage[(used_by, stage_number)].add(Totals(hash_bits=1))

        stage_tot.add(Totals(hash_bits=len(bits)))

        # ------------------------------------------------------------------------------
        hash_dist_units = get_attr(HASH_DISTRIBUTION_UNITS, mau_stage)
        nHashIds = get_attr(NHASHIDS, hash_dist_units)
        nUnitIds = get_attr(NUNITIDS, hash_dist_units)
        stage_avail.add(Totals(hash_dist_units=nHashIds * nUnitIds))
        units = get_attr(UNITS, hash_dist_units)
        cnt = 0
        for unit in units:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if used_by == HASH_PARITY_BIT:  # Don't want this to show up in 'tables' table
                    continue
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(hash_dist_units=1))

        stage_tot.add(Totals(hash_dist_units=cnt))

        # ------------------------------------------------------------------------------
        gateways = get_attr(GATEWAYS, mau_stage)
        nRows = get_attr(NROWS, gateways)
        nUnits = get_attr(NUNITS, gateways)
        stage_avail.add(Totals(gateway=nRows * nUnits))
        gw = get_attr(GATEWAYS, gateways)
        cnt = 0
        for unit in gw:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(gateway=1))

        stage_tot.add(Totals(gateway=cnt))

        # ------------------------------------------------------------------------------
        rams = get_attr(RAMS, mau_stage)
        nRows = get_attr(NROWS, rams)
        nColumns = get_attr(NCOLUMNS, rams)
        stage_avail.add(Totals(sram=nRows * nColumns))
        srams = get_attr(SRAMS, rams)
        cnt = 0
        for unit in srams:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(sram=1))

        stage_tot.add(Totals(sram=cnt))

        # ------------------------------------------------------------------------------
        map_rams = get_attr(MAP_RAMS, mau_stage)
        nRows = get_attr(NROWS, map_rams)
        nUnits = get_attr(NUNITS, map_rams)
        stage_avail.add(Totals(map_ram=nRows * nUnits))
        maprams = get_attr(MAPRAMS, map_rams)
        cnt = 0
        for unit in maprams:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(map_ram=1))

        stage_tot.add(Totals(map_ram=cnt))

        # ------------------------------------------------------------------------------
        tcams = get_attr(TCAMS, mau_stage)
        nRows = get_attr(NROWS, tcams)
        nColumns = get_attr(NCOLUMNS, tcams)
        stage_avail.add(Totals(tcam=nRows * nColumns))
        t = get_attr(TCAMS, tcams)
        cnt = 0
        for unit in t:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(tcam=1))

        stage_tot.add(Totals(tcam=cnt))

        # ------------------------------------------------------------------------------
        vliw = get_attr(VLIW, mau_stage)
        size = get_attr(SIZE, vliw)
        stage_avail.add(Totals(vliw=size))
        instructions = get_attr(INSTRUCTIONS, vliw)
        cnt = 0
        for unit in instructions:
            cnt += 1
            color_usages = get_attr(COLOR_USAGES, unit)
            for c in color_usages:
                usages = get_attr(USAGES, c)
                for u in usages:
                    if not has_attr(USED_BY, u):
                        print(str(color_usages))
                    used_by = get_attr(USED_BY, u)
                    if used_by == UNUSED:  # Skip
                        continue
                    if (used_by, stage_number) not in table_name_stage:
                        table_name_stage[(used_by, stage_number)] = Totals()
                    table_name_stage[(used_by, stage_number)].add(Totals(vliw=1))

        stage_tot.add(Totals(vliw=cnt))

        # ------------------------------------------------------------------------------
        meter_alus = get_attr(METER_ALUS, mau_stage)
        nAlus = get_attr(NALUS, meter_alus)
        stage_avail.add(Totals(meter_alu=nAlus))
        meters = get_attr(METERS, meter_alus)
        cnt = 0
        for unit in meters:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(meter_alu=1))

        stage_tot.add(Totals(meter_alu=cnt))

        # ------------------------------------------------------------------------------
        stats_alus = get_attr(STATISTICS_ALUS, mau_stage)
        nAlus = get_attr(NALUS, stats_alus)
        stage_avail.add(Totals(stats_alu=nAlus))
        stats = get_attr(STATS, stats_alus)
        cnt = 0
        for unit in stats:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(stats_alu=1))

        stage_tot.add(Totals(stats_alu=cnt))

        # ------------------------------------------------------------------------------
        stashes = get_attr(STASHES, mau_stage)
        nRows = get_attr(NROWS, stashes)
        nUnits = get_attr(NUNITS, stashes)
        stage_avail.add(Totals(stash=nRows * nUnits))
        s = get_attr(STASHES, stashes)
        cnt = 0
        for unit in s:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(stash=1))

        stage_tot.add(Totals(stash=cnt))

        # ------------------------------------------------------------------------------
        exm_search_buses = get_attr(EXM_SEARCH_BUSES, mau_stage)
        size = get_attr(SIZE, exm_search_buses)
        stage_avail.add(Totals(exm_search_bus=size))
        ids = get_attr(IDS, exm_search_buses)
        cnt = 0
        for unit in ids:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(exm_search_bus=1))

        stage_tot.add(Totals(exm_search_bus=cnt))

        # ------------------------------------------------------------------------------
        exm_result_buses = get_attr(EXM_RESULT_BUSES, mau_stage)
        size = get_attr(SIZE, exm_result_buses)
        stage_avail.add(Totals(exm_result_bus=size))
        ids = get_attr(IDS, exm_result_buses)
        cnt = 0
        for unit in ids:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(exm_result_bus=1))

        stage_tot.add(Totals(exm_result_bus=cnt))

        # ------------------------------------------------------------------------------
        tind_result_buses = get_attr(TIND_RESULT_BUSES, mau_stage)
        size = get_attr(SIZE, tind_result_buses)
        stage_avail.add(Totals(tind_result_bus=size))
        ids = get_attr(IDS, tind_result_buses)
        cnt = 0
        for unit in ids:
            cnt += 1
            usages = get_attr(USAGES, unit)
            for u in usages:
                used_by = get_attr(USED_BY, u)
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(tind_result_bus=1))

        stage_tot.add(Totals(tind_result_bus=cnt))

        # ------------------------------------------------------------------------------
        action_bus_bytes = get_attr(ACTION_BUS_BYTES, mau_stage)
        size = get_attr(SIZE, action_bus_bytes)
        stage_avail.add(Totals(action_data_bytes=size))
        bytes = get_attr(BYTES, action_bus_bytes)
        cnt = 0
        for unit in bytes:
            cnt += 1
            used_by_tables = get_attr(USED_BY_TABLES, unit)
            for used_by in used_by_tables:
                if (used_by, stage_number) not in table_name_stage:
                    table_name_stage[(used_by, stage_number)] = Totals()
                table_name_stage[(used_by, stage_number)].add(Totals(action_data_bytes=1))

        stage_tot.add(Totals(action_data_bytes=cnt))

        # ------------------------------------------------------------------------------
        action_slots = get_attr(ACTION_SLOTS, mau_stage)
        for slot in action_slots:
            bw = get_attr(SLOT_BIT_WIDTH, slot)
            avail = get_attr(MAXIMUM_SLOTS, slot)
            used = get_attr(NUMBER_USED, slot)

            if bw == 8:
                stage_avail.add(Totals(action_slots_8=avail))
                stage_tot.add(Totals(action_slots_8=used))
            elif bw == 16:
                stage_avail.add(Totals(action_slots_16=avail))
                stage_tot.add(Totals(action_slots_16=used))
            elif bw == 32:
                stage_avail.add(Totals(action_slots_32=avail))
                stage_tot.add(Totals(action_slots_32=used))
            else:
                assert False, "Unexpected action slot bit width '%s'" % str(bw)

        # ------------------------------------------------------------------------------
        logical_tables = get_attr(LOGICAL_TABLES, mau_stage)
        size = get_attr(SIZE, logical_tables)
        stage_avail.add(Totals(logical_table_id=size))
        ids = get_attr(IDS, logical_tables)
        cnt = 0
        for unit in ids:
            cnt += 1
            used_by = get_attr(TABLE_NAME, unit)
            if (used_by, stage_number) not in table_name_stage:
                table_name_stage[(used_by, stage_number)] = Totals()
            table_name_stage[(used_by, stage_number)].add(Totals(logical_table_id=1))

        stage_tot.add(Totals(logical_table_id=cnt))

    return stage_totals, stage_available, table_name_stage


def build_table(data_dict, total_available=None, tables=False, ingress_names=None):
    hdrs = [
        [
            "Stage Number",
            "Exact Match Input xbar",
            "Ternary Match Input xbar",
            "Hash Bit",
            "Hash Dist Unit",
            "Gateway",
            "SRAM",
            "Map RAM",
            "TCAM",
            "VLIW Instr",
            "Meter ALU",
            "Stats ALU",
            "Stash",
            "Exact Match Search Bus",
            "Exact Match Result Bus",
            "Tind Result Bus",
            "Action Data Bus Bytes",
            "8-bit Action Slots",
            "16-bit Action Slots",
            "32-bit Action Slots",
            "Logical TableID",
        ]
    ]

    if tables:
        hdrs = [
            [
                "Table",
                "Stage",
                "Crossbar",
                "Hash",
                "Gateways",
                "RAMs",
                "TCAMs",
                "Map",
                "Action",
                "VLIW",
                "Exm",
                "Exm",
                "Tind",
            ],
            [
                "Name",
                "Number",
                "Bytes",
                "Bits",
                "",
                "",
                "",
                "RAMs",
                "Data",
                "Slots",
                "Search",
                "Result",
                "Result",
            ],
            ["", "", "", "", "", "", "", "", "Bus", "", "Bus", "Bus", "Bus"],
            ["", "", "", "", "", "", "", "", "Bytes", "", "", "", ""],
        ]

    def gress_sort(table_name):
        if ingress_names is not None:
            if table_name in ingress_names:
                return 0
        return 1

    data = []
    final_total = Totals()
    final_available = Totals()
    for k in data_dict:
        totals = data_dict[k]
        available = None
        if total_available is not None:
            available = total_available[k]
            final_available.add(available)
        final_total.add(totals)
        line = []
        if tables:
            line.append(k[0])
            line.append(k[1])
        else:
            line.append(k)
        line.extend(totals.get_row(available, tables=tables))
        data.append(line)

    if not tables:
        blank = []
        for _ in hdrs[0]:
            blank.append("")
        data.append(blank)

        if total_available is None:
            fline = ["Totals"]
            fline.extend(final_total.get_row())
        else:
            fline = ["Average"]
            fline.extend(final_total.get_row(final_available))

        data.append(fline)

    if tables:
        # Sort by stage, gress (ingress first), table name
        # data.sort(key=lambda x: (x[1], gress_sort(x[0]), x[0]))
        # Sort by stage, table name
        data.sort(key=lambda x: (x[1], x[0]))

    log = logging.getLogger(LOGGER_NAME)
    log.info(print_table(hdrs, data))


def produce_mau_resources(source, output):
    # Setup logging
    log_file = os.path.join(output, RESULTS_FILE)
    logging_file_handle = logging.FileHandler(log_file, mode='w')
    log = logging.getLogger(LOGGER_NAME)
    log.setLevel(logging.INFO)
    log.propagate = 0
    logging_file_handle.setLevel(logging.INFO)
    log.addHandler(logging_file_handle)

    # Load context.json to get gress assignments for tables
    # json_file_ctx = open(source2, 'r')
    # context = json.load(json_file_ctx)
    # ingress_names = _parse_context_json(context)
    # # Tidy up
    # json_file_ctx.close()
    ingress_names = None

    # Load JSON and make sure versions are correct
    json_file = open(source, 'r')
    try:
        context = json.load(json_file)
    except:
        json_file.close()
        context = {}
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
    stage_totals, stage_available, table_name_stage = _parse_resources_json(context)

    # Output summary table in log file
    log.info("")
    build_table(stage_totals)
    log.info("")
    build_table(stage_totals, stage_available)
    log.info("\nAllocated Resource Usage")
    build_table(table_name_stage, tables=True, ingress_names=ingress_names)

    # Tidy up
    json_file.close()

    return SUCCESS, "ok"


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument(
        'source', metavar='source', type=str, help='The input resources.json source file to use.'
    )
    # parser.add_argument('source2', metavar='source2', type=str,
    #                     help='The input context.json source file to use.')
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
        return_code, err_msg = produce_mau_resources(args.source, args.output)
    except:
        print_error_and_exit("Unable to create %s." % RESULTS_FILE)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

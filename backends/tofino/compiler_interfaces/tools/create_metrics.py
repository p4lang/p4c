#!/usr/bin/env python3

"""
This script produces a metrics file from an input resources.json file.
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
from schemas.metrics_schema import MetricsJSONSchema
from schemas.schema_keys import *
from schemas.schema_enum_values import *

# The minimum resources.json schema version required.
MINIMUM_RESOURCES_JSON_REQUIRED = "1.0.5"
# The minimum phv.json schema version required.
MINIMUM_PHV_JSON_REQUIRED = "2.0.0"
# The minimum context.json schema version required.
MINIMUM_CONTEXT_JSON_REQUIRED = "1.7.0"
# The minimum power.json schema version required.
MINIMUM_POWER_JSON_REQUIRED = "1.0.0"
# The minimum manifest.json schema version required.
MINIMUM_MANIFEST_JSON_REQUIRED = "2.0.0"
RESULTS_JSON = "metrics.json"
METRICS_SCHEMA_VERSION = "1.1.0"  # the schema version produced
DIV = "-"*40


class ContainerUsage(object):
    def __init__(self, address, container_type):
        self.address = address
        self.container_type = container_type
        self.bit_cnt = {}  # dictionary from bit number to cnt of how many times it's used

    def using_bit(self, bit):
        if bit not in self.bit_cnt:
            self.bit_cnt[bit] = 0
        self.bit_cnt[bit] += 1

    def __str__(self):
        to_ret = "PHV %d" % self.address
        for b in sorted(self.bit_cnt):
            to_ret += "\n  bit %d: %d" % (b, self.bit_cnt[b])
        return to_ret

    def get_bits_used(self):
        return len(self.bit_cnt)

    def get_bits_overlaid(self):
        cnt = 0
        for b in self.bit_cnt:
            if self.bit_cnt[b] > 1:
                cnt += 1
        return cnt


class PhvUsage(object):
    def __init__(self, normal=None):
        if normal is None:
            # Keep per-bit-width Container usage information.
            # This is a dictionary that maps container bit width to a dictionary
            # that maps container addresses to ContainerUsage object.
            self.normal = OrderedDict()
            self.normal[8] = OrderedDict()
            self.normal[16] = OrderedDict()
            self.normal[32] = OrderedDict()
        else:
            self.normal = normal

    def _print_usage(self, dict_name, which_dict):
        to_ret = "\n  %s:" % str(dict_name)
        for b in which_dict:
            to_ret += "\n    %d-bit: %d" % (b, len(which_dict[b]))
        return to_ret

    def __str__(self):
        to_ret = "%s\nPHV Usage\n%s" % (DIV, DIV)
        to_ret += self._print_usage("Normal", self.normal)
        return to_ret

    def _cnt_bits(self, some_dict, bit_width):
        bits_used = 0
        bits_overlaid = 0
        for address in some_dict[bit_width]:
            cusage = some_dict[bit_width][address]
            bits_used += cusage.get_bits_used()
            bits_overlaid += cusage.get_bits_overlaid()
        return bits_used, bits_overlaid

    def _ctype_json(self, which_dict):
        c = []
        for b in which_dict:
            d = OrderedDict()
            d["bit_width"] = b
            bits_used, bits_overlaid = self._cnt_bits(which_dict, b)
            d["containers_occupied"] = len(which_dict[b])
            d["bits_occupied"] = bits_used
            d["bits_overlaid"] = bits_overlaid
            c.append(d)
        return c

    def to_json(self):
        to_ret = OrderedDict()
        to_ret["normal"] = self._ctype_json(self.normal)
        return to_ret

    def _add(self, self_dict, other_dict):
        for b in self_dict:
            if b in other_dict:
                for address in other_dict[b]:
                    if address in self_dict[b]:
                        for b in other_dict[b][address]:
                            self_dict[b][address].using_bit(b)
                    else:
                        self_dict[b][address] = other_dict[b][address]

    def add(self, other):
        self._add(self.normal, other.normal)


class TofinoPhvUsage(PhvUsage):
    def __init__(self, normal=None, tagalong=None):
        PhvUsage.__init__(self, normal=normal)
        if tagalong is None:
            self.tagalong = OrderedDict()
            self.tagalong[8] = OrderedDict()
            self.tagalong[16] = OrderedDict()
            self.tagalong[32] = OrderedDict()
        else:
            self.tagalong = tagalong

    def __str__(self):
        to_ret = PhvUsage.__init__(self)
        to_ret += self._print_usage("Tagalong", self.tagalong)
        return to_ret

    def to_json(self):
        to_ret = PhvUsage.to_json(self)
        to_ret["tagalong"] = self._ctype_json(self.tagalong)
        return to_ret

    def add(self, other):
        PhvUsage.add(self, other)
        self._add(self.tagalong, other.tagalong)


class Tofino2PhvUsage(PhvUsage):
    def __init__(self, normal=None, mocha=None, dark=None):
        PhvUsage.__init__(self, normal=normal)
        if mocha is None:
            self.mocha = OrderedDict()
            self.mocha[8] = OrderedDict()
            self.mocha[16] = OrderedDict()
            self.mocha[32] = OrderedDict()
        else:
            self.mocha = mocha
        if dark is None:
            self.dark = OrderedDict()
            self.dark[8] = OrderedDict()
            self.dark[16] = OrderedDict()
            self.dark[32] = OrderedDict()
        else:
            self.dark = dark

    def __str__(self):
        to_ret = PhvUsage.__init__(self)
        to_ret += self._print_usage("Mocha", self.mocha)
        to_ret += self._print_usage("Dark", self.dark)
        return to_ret

    def to_json(self):
        to_ret = PhvUsage.to_json(self)
        to_ret["mocha"] = self._ctype_json(self.mocha)
        to_ret["dark"] = self._ctype_json(self.dark)
        return to_ret

    def add(self, other):
        PhvUsage.add(self, other)
        self._add(self.mocha, other.mocha)
        self._add(self.dark, other.dark)


class ParserUsage(object):
    def __init__(self, tcam_rows=None):
        if tcam_rows is None:
            self.tcam_rows = OrderedDict()
            self.tcam_rows[INGRESS] = 0
            self.tcam_rows[EGRESS] = 0
        else:
            self.tcam_rows = tcam_rows

    def __str__(self):
        to_ret = "%s\nParser Usage\n%s" % (DIV, DIV)
        for g in [INGRESS, EGRESS]:
            to_ret += "\n%s:" % g
            to_ret += "\n  TCAM rows: %d" % self.tcam_rows[g]
        return to_ret

    def to_json(self):
        to_ret = OrderedDict()
        i = OrderedDict()
        e = OrderedDict()

        for g, d in [(INGRESS, i), (EGRESS, e)]:
            d["tcam_rows"] = self.tcam_rows[g]

        to_ret[INGRESS] = i
        to_ret[EGRESS] = e
        return to_ret

    def add(self, other):
        for g in self.tcam_rows:
            self.tcam_rows[g] += other.tcam_rows[g]

class MauUsage(object):
    def __init__(self, resource_cnt=None):
        # dictionary that maps resource name to total usage
        self.resource_count = {}
        # required resources that are part of metrics schema
        req = [SRAMS, TCAMS, MAP_RAMS, LOGICAL_TABLES, ACTION_BUS_BYTES, EXACT_CROSSBAR_BYTES, TERNARY_CROSSBAR_BYTES]
        for resource in req:
            self.resource_count[resource] = 0
        if resource_cnt is not None:
            for resource in resource_cnt:
                self.resource_count[resource] = resource_cnt[resource]
        self.latency = {INGRESS: 0, EGRESS: 0, GHOST: 0}
        self.power = {INGRESS: 0.0, EGRESS: 0.0, GHOST: 0.0}

    def __str__(self):
        to_ret = "%s\nMAU Usage\n%s" % (DIV, DIV)
        props = [("SRAM", self.resource_count[SRAMS]),
                 ("TCAM", self.resource_count[TCAMS]),
                 ("MapRAM", self.resource_count[MAP_RAMS]),
                 ("Logical Tables", self.resource_count[LOGICAL_TABLES]),
                 ("Action Data Bus Bytes", self.resource_count[ACTION_BUS_BYTES]),
                 ("Exact Match Crossbar", self.resource_count[EXACT_CROSSBAR_BYTES]),
                 ("Ternary Match Crossbar", self.resource_count[TERNARY_CROSSBAR_BYTES])]
        # TODO: Add others as see fit
        for name, value in props:
            to_ret += "\n  %s: %d" % (name, value)
        return to_ret

    def to_json(self, target):
        to_ret = OrderedDict()
        to_ret[SRAMS] = self.resource_count[SRAMS]
        to_ret[TCAMS] = self.resource_count[TCAMS]
        to_ret[MAP_RAMS] = self.resource_count[MAP_RAMS]
        to_ret[LOGICAL_TABLES] = self.resource_count[LOGICAL_TABLES]
        to_ret[ACTION_BUS_BYTES] = self.resource_count[ACTION_BUS_BYTES]
        to_ret[EXACT_CROSSBAR_BYTES] = self.resource_count[EXACT_CROSSBAR_BYTES]
        to_ret[TERNARY_CROSSBAR_BYTES] = self.resource_count[TERNARY_CROSSBAR_BYTES]

        lat = []
        pwr = []
        pk = [INGRESS, EGRESS]
        if target != TARGET.TOFINO:
            pk.append(GHOST)

        for g in pk:
            glat = OrderedDict()
            glat[GRESS] = g
            glat[CYCLES] = self.latency[g]
            lat.append(glat)

            gpwr = OrderedDict()
            gpwr[GRESS] = g
            gpwr[ESTIMATE] = str(self.power[g])
            pwr.append(gpwr)

        to_ret[LATENCY] = lat
        to_ret[POWER] = pwr

        return to_ret

    def add(self, other):
        for resource in other.resource_count:
            if resource in self.resource_count:
                self.resource_count[resource] += other.resource_count[resource]
            else:
                self.resource_count[resource] = other.resource_count[resource]

    def update_latency(self, gress, value):
        self.latency[gress] = value

    def update_power(self, gress, value):
        self.power[gress] = value

class DeparserUsage(object):
    def __init__(self, pov_bits=None, fde_entries=None):
        if pov_bits is None:
            self.pov_bits = OrderedDict()
            self.pov_bits[INGRESS] = 0
            self.pov_bits[EGRESS] = 0
        else:
            self.pov_bits = pov_bits
        if fde_entries is None:
            self.fde_entries = OrderedDict()
            self.fde_entries[INGRESS] = 0
            self.fde_entries[EGRESS] = 0
        else:
            self.fde_entries = fde_entries

    def __str__(self):
        to_ret = "%s\nDeparser Usage\n%s" % (DIV, DIV)
        for g in [INGRESS, EGRESS]:
            to_ret += "\n%s:" % g
            to_ret += "\n  POV bits: %d" % self.pov_bits[g]
            to_ret += "\n  Field Dictionary Entries: %d" % self.fde_entries[g]
        return to_ret

    def to_json(self):
        to_ret = OrderedDict()
        i = OrderedDict()
        e = OrderedDict()

        for g, d in [(INGRESS, i), (EGRESS, e)]:
            d["pov_bits"] = self.pov_bits[g]
            d["fde_entries"] = self.fde_entries[g]

        to_ret[INGRESS] = i
        to_ret[EGRESS] = e
        return to_ret

    def add(self, other):
        for g in self.pov_bits:
            self.pov_bits[g] += other.pov_bits[g]
        for g in self.fde_entries:
            self.fde_entries[g] += other.fde_entries[g]

class ClotUsage(object):
    def __init__(self):
        self.all_allocated_bits = OrderedDict()
        self.allocated_bits = OrderedDict()
        self.unallocated_bits = OrderedDict()
        self.redundant_phv_bits = OrderedDict()

        for gress in [INGRESS, EGRESS]:
            self.all_allocated_bits[gress] = 0
            self.allocated_bits[gress] = 0
            self.unallocated_bits[gress] = 0
            self.redundant_phv_bits[gress] = 0

    def __str__(self):
        result = "%s\nCLOT Usage\n%s" % (DIV, DIV)
        for gress in [INGRESS, EGRESS]:
            result += "\n%s: " % gress
            result += "\n  All allocated bits: %d" % self.all_allocated_bits[gress]
            result += "\n  Allocated bits: %d" % self.allocated_bits[gress]
            result += "\n  Unallocated bits: %d" % self.unallocated_bits[gress]
            result += "\n  Redundant PHV bits: %d" % self.redundant_phv_bits[gress]
        return result

    def to_json(self):
        result = OrderedDict()
        for gress in [INGRESS, EGRESS]:
            d = OrderedDict()
            d["all_allocated_bits"] = self.all_allocated_bits[gress]
            d["allocated_bits"] = self.allocated_bits[gress]
            d["unallocated_bits"] = self.unallocated_bits[gress]
            d["redundant_phv_bits"] = self.redundant_phv_bits[gress]
            result[gress] = d
        return result

    def add(self, gress, field):
        bit_width = get_attr(BIT_WIDTH, field)
        num_bits_in_clots = get_attr(NUM_BITS_IN_CLOTS, field)
        num_bits_in_phvs = get_attr(NUM_BITS_IN_PHVS, field)

        self.all_allocated_bits[gress] += num_bits_in_clots

        # Only count the field if it is unused.
        if get_attr(IS_READONLY, field) \
                or get_attr(IS_MODIFIED, field) \
                or get_attr(IS_CHECKSUM, field):
            return

        self.allocated_bits[gress] += num_bits_in_clots
        self.unallocated_bits[gress] += bit_width - num_bits_in_clots
        self.redundant_phv_bits[gress] += num_bits_in_clots + num_bits_in_phvs - bit_width

# ----------------------------------------
#  Produce log file
# ----------------------------------------

def _parse_manifest_json(context):

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_MANIFEST_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported manifest.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_MANIFEST_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    compilation_time = get_attr(COMPILATION_TIME, context)

    return compilation_time

def _parse_power_json(context):

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_POWER_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported power.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_POWER_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    mau_latency = OrderedDict()
    mau_power = OrderedDict()

    total_latency = get_attr(TOTAL_LATENCY, context)
    for d in total_latency:
        gress = get_attr(GRESS, d)
        latency = get_attr(LATENCY, d)
        # pipe = get_attr(PIPE_NUMBER, d)
        mau_latency[gress] = latency

    total_power = get_attr(TOTAL_POWER, context)
    for d in total_power:
        gress = get_attr(GRESS, d)
        power = get_attr(POWER, d)
        # pipe = get_attr(PIPE_NUMBER, d)
        mau_power[gress] = power

    return mau_latency, mau_power

def _parse_context_json(context):

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_CONTEXT_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported context.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_CONTEXT_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    target = get_attr(TARGET_KEY, context)

    return target

def _parse_phv_json(context):

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_PHV_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported phv.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_PHV_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    target = get_attr(TARGET_KEY, context)
    if target == TARGET.TOFINO:
        phv_usage = TofinoPhvUsage()
    else:
        phv_usage = Tofino2PhvUsage()

    summary = {}  # dictionary from container type to dictionary from sizes to dictionary of address to ContainerUsage

    containers = get_attr(CONTAINERS, context)
    for c in containers:
        bit_width = get_attr(BIT_WIDTH, c)
        container_type = get_attr(CONTAINER_TYPE, c)
        phv_number = get_attr(PHV_NUMBER, c)
        slices = get_attr(SLICES, c)

        if container_type not in summary:
            summary[container_type] = {}
        if bit_width not in summary[container_type]:
            summary[container_type][bit_width] = {}
        if phv_number not in summary[container_type][bit_width]:
            summary[container_type][bit_width][phv_number] = ContainerUsage(phv_number, container_type)

        cusage = summary[container_type][bit_width][phv_number]
        for r in slices:
            phv_slice_info = get_attr(SLICE_INFO, r)
            phv_lsb = get_attr(LSB, phv_slice_info)
            phv_msb = get_attr(MSB, phv_slice_info)
            for b in list(range(phv_lsb, phv_msb + 1)):
                cusage.using_bit(b)

    if NORMAL in summary:
        if target == TARGET.TOFINO:
            u = TofinoPhvUsage(normal=summary[NORMAL])
        else:
            u = Tofino2PhvUsage(normal=summary[NORMAL])
        phv_usage.add(u)

    if TAGALONG in summary:
        u = TofinoPhvUsage(tagalong=summary[TAGALONG])
        phv_usage.add(u)

    if MOCHA in summary:
        u = Tofino2PhvUsage(mocha=summary[MOCHA])
        phv_usage.add(u)

    if DARK in summary:
        u = Tofino2PhvUsage(dark=summary[DARK])
        phv_usage.add(u)

    return phv_usage

def _parse_resources_json(context):

    parser_usage = ParserUsage()
    mau_usage = MauUsage()
    deparser_usage = DeparserUsage()
    clot_usage = ClotUsage()

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_RESOURCES_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported resources.json schema version %s.\n" % str(schema_version)
        error_msg += "Required minimum version is: %s" % str(MINIMUM_RESOURCES_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    resources = get_attr(RESOURCES, context)

    parser = get_attr(PARSER, resources)
    parsers = get_attr(PARSERS, parser)
    for par in parsers:
        gress = get_attr(GRESS, par)
        states = get_attr(STATES, par)

        s = OrderedDict()
        s[INGRESS] = 0
        s[EGRESS] = 0
        s[gress] = len(states)
        parser_usage.add(ParserUsage(tcam_rows=s))


    mau = get_attr(MAU, resources)
    mau_stages = get_attr(MAU_STAGES, mau)

    for mau_stage in mau_stages:
        stage_number = get_attr(STAGE_NUMBER, mau_stage)

        #------------------------------------------------------------------------------
        xbar_bytes = get_attr(XBAR_BYTES, mau_stage)
        bytes = get_attr(BYTES, xbar_bytes)
        type_to_cnt = {EXACT: 0, TERNARY: 0}
        for b in bytes:
            byte_type = get_attr(BYTE_TYPE, b)
            type_to_cnt[byte_type] += 1
        rcnt = {EXACT_CROSSBAR_BYTES: type_to_cnt[EXACT], TERNARY_CROSSBAR_BYTES: type_to_cnt[TERNARY]}
        mau_usage.add(MauUsage(rcnt))

        # ------------------------------------------------------------------------------
        hash_bits = get_attr(HASH_BITS, mau_stage)
        bits = get_attr(BITS, hash_bits)
        mau_usage.add(MauUsage({HASH_BITS: len(bits)}))

        # ------------------------------------------------------------------------------
        hash_dist_units = get_attr(HASH_DISTRIBUTION_UNITS, mau_stage)
        units = get_attr(UNITS, hash_dist_units)
        mau_usage.add(MauUsage({HASH_DISTRIBUTION_UNITS: len(units)}))

        # ------------------------------------------------------------------------------
        gateways = get_attr(GATEWAYS, mau_stage)
        gw = get_attr(GATEWAYS, gateways)
        mau_usage.add(MauUsage({GATEWAYS: len(gw)}))

        # ------------------------------------------------------------------------------
        rams = get_attr(RAMS, mau_stage)
        srams = get_attr(SRAMS, rams)
        mau_usage.add(MauUsage({SRAMS: len(srams)}))

        # ------------------------------------------------------------------------------
        map_rams = get_attr(MAP_RAMS, mau_stage)
        maprams = get_attr(MAPRAMS, map_rams)
        mau_usage.add(MauUsage({MAP_RAMS: len(maprams)}))

        # ------------------------------------------------------------------------------
        tcams = get_attr(TCAMS, mau_stage)
        t = get_attr(TCAMS, tcams)
        mau_usage.add(MauUsage({TCAMS: len(t)}))

        # ------------------------------------------------------------------------------
        vliw = get_attr(VLIW, mau_stage)
        instructions = get_attr(INSTRUCTIONS, vliw)
        mau_usage.add(MauUsage({VLIW: len(instructions)}))

        # ------------------------------------------------------------------------------
        meter_alus = get_attr(METER_ALUS, mau_stage)
        meters = get_attr(METERS, meter_alus)
        mau_usage.add(MauUsage({METER_ALUS: len(meters)}))

        # ------------------------------------------------------------------------------
        stats_alus = get_attr(STATISTICS_ALUS, mau_stage)
        stats = get_attr(STATS, stats_alus)
        mau_usage.add(MauUsage({STATISTICS_ALUS: len(stats)}))

        # ------------------------------------------------------------------------------
        stashes = get_attr(STASHES, mau_stage)
        s = get_attr(STASHES, stashes)
        mau_usage.add(MauUsage({STASHES: len(s)}))

        # ------------------------------------------------------------------------------
        exm_search_buses = get_attr(EXM_SEARCH_BUSES, mau_stage)
        ids = get_attr(IDS, exm_search_buses)
        mau_usage.add(MauUsage({EXM_SEARCH_BUSES: len(ids)}))

        # ------------------------------------------------------------------------------
        exm_result_buses = get_attr(EXM_RESULT_BUSES, mau_stage)
        ids = get_attr(IDS, exm_result_buses)
        mau_usage.add(MauUsage({EXM_RESULT_BUSES: len(ids)}))

        # ------------------------------------------------------------------------------
        tind_result_buses = get_attr(TIND_RESULT_BUSES, mau_stage)
        ids = get_attr(IDS, tind_result_buses)
        mau_usage.add(MauUsage({TIND_RESULT_BUSES: len(ids)}))

        # ------------------------------------------------------------------------------
        action_bus_bytes = get_attr(ACTION_BUS_BYTES, mau_stage)
        bytes = get_attr(BYTES, action_bus_bytes)
        mau_usage.add(MauUsage({ACTION_BUS_BYTES: len(bytes)}))

        # ------------------------------------------------------------------------------
        action_slots = get_attr(ACTION_SLOTS, mau_stage)
        for slot in action_slots:
            bw = get_attr(SLOT_BIT_WIDTH, slot)
            # avail = get_attr(MAXIMUM_SLOTS, slot)
            used = get_attr(NUMBER_USED, slot)

            if bw == 8:
                mau_usage.add(MauUsage({"%s_8" % ACTION_SLOTS: used}))
            elif bw == 16:
                mau_usage.add(MauUsage({"%s_16" % ACTION_SLOTS: used}))
            elif bw == 32:
                mau_usage.add(MauUsage({"%s_32" % ACTION_SLOTS: used}))
            else:
                print_error_and_exit("Unexpected action slot bit width '%s'" % str(bw))

        # ------------------------------------------------------------------------------
        logical_tables = get_attr(LOGICAL_TABLES, mau_stage)
        ids = get_attr(IDS, logical_tables)
        mau_usage.add(MauUsage({LOGICAL_TABLES: len(ids)}))

    deparser = get_attr(DEPARSER, resources)
    for g in deparser:
        gress = get_attr(GRESS, g)
        pov = get_attr(POV, g)
        pov_bits = get_attr(POV_BITS, pov)
        p = OrderedDict()
        p[INGRESS] = 0
        p[EGRESS] = 0
        p[gress] = len(pov_bits)
        deparser_usage.add(DeparserUsage(pov_bits=p))
        fde_entries = get_attr(FDE_ENTRIES, g)
        f = OrderedDict()
        f[INGRESS] = 0
        f[EGRESS] = 0
        f[gress] = len(fde_entries)
        deparser_usage.add(DeparserUsage(fde_entries=f))

    if has_attr(CLOTS, resources):
        clots = get_attr(CLOTS, resources)
        for gress in [INGRESS, EGRESS]:
            for clot in clots:
                if gress != get_attr(GRESS, clot): continue
                fields = get_attr(CLOT_ELIGIBLE_FIELDS, clot)
                for field in fields:
                    clot_usage.add(gress, field)

    compiler_version = get_attr(COMPILER_VERSION, context)
    build_date = get_attr(BUILD_DATE, context)
    run_id = get_attr(RUN_ID, context)
    program_name = get_attr(PROGRAM_NAME, context)

    return compiler_version, build_date, run_id, program_name, \
           parser_usage, mau_usage, deparser_usage, clot_usage


def produce_metrics(context_source, resources_source, phv_source, power_source, manifest_source, output):
    # Note: power_source and manifest_source are allowed to be None.

    # Load and parse context JSON
    try:
        context_json_file = open(context_source, 'r')
        context = json.load(context_json_file)
        context_json_file.close()
    except:
        print_error_and_exit("Unable to open JSON file '%s'" % str(context_source))
    target = _parse_context_json(context)

    # Load and parse resources JSON
    try:
        resources_json_file = open(resources_source, 'r')
        resources_context = json.load(resources_json_file)
        resources_json_file.close()
    except:
        print_error_and_exit("Unable to open JSON file '%s'" % str(resources_source))
    compiler_version, build_date, run_id, program_name, \
        parser_usage, mau_usage, deparser_usage, clot_usage = _parse_resources_json(resources_context)

    # Load and parse phv JSON
    try:
        phv_json_file = open(phv_source, 'r')
        phv_context = json.load(phv_json_file)
        phv_json_file.close()
    except:
        print_error_and_exit("Unable to open JSON file '%s'" % str(phv_source))
    phv_usage = _parse_phv_json(phv_context)

    mau_latency = {}
    mau_power = {}
    # The power.json input is allowed to be unspecified.
    if power_source is not None:
        # Load and parse power JSON
        try:
            power_json_file = open(power_source, 'r')
            power_context = json.load(power_json_file)
            power_json_file.close()
        except:
            print_error_and_exit("Unable to open JSON file '%s'" % str(power_source))
        mau_latency, mau_power = _parse_power_json(power_context)
        for g in mau_latency:
            mau_usage.update_latency(g, mau_latency[g])
        for g in mau_power:
            mau_usage.update_power(g, mau_power[g])

    compilation_time = "unknown"
    # The manifest.json input is allowed to be unspecified.
    if manifest_source is not None:
        # Load and parse manifest JSON
        try:
            manifest_json_file = open(manifest_source, 'r')
            manifest_context = json.load(manifest_json_file)
            manifest_json_file.close()
        except:
            print_error_and_exit("Unable to open JSON file '%s'" % str(manifest_source))
        compilation_time = _parse_manifest_json(manifest_context)


    # Dump JSON
    metric_json = OrderedDict()
    metric_json[PROGRAM_NAME] = program_name
    metric_json[BUILD_DATE] = build_date
    metric_json[RUN_ID] = str(run_id)
    metric_json[COMPILER_VERSION] = str(compiler_version)
    metric_json[SCHEMA_VERSION] = METRICS_SCHEMA_VERSION
    metric_json[TARGET_KEY] = target

    metric_json[COMPILATION_TIME] = compilation_time
    metric_json[PHV] = phv_usage.to_json()
    metric_json[PARSER] = parser_usage.to_json()
    metric_json[MAU] = mau_usage.to_json(target)
    metric_json[DEPARSER] = deparser_usage.to_json()
    metric_json[CLOTS] = clot_usage.to_json()

    cpath = os.path.join(output, RESULTS_JSON)
    with open(cpath, "w") as cfile:
        json.dump(metric_json, cfile, indent=4)

    metrics_schema = MetricsJSONSchema.get_schema()
    jsonschema.validate(metric_json, metrics_schema)
    return SUCCESS, "ok"


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument('context', metavar='context', type=str,
                        help='The input context.json source file to use.')
    parser.add_argument('resources', metavar='resources', type=str,
                        help='The input resources.json source file to use.')
    parser.add_argument('phv', metavar='phv', type=str,
                        help='The input phv.json source file to use.')
    parser.add_argument('power', metavar='power', type=str,
                        help='The input power.json source file to use.')
    parser.add_argument('manifest', metavar='manifest', type=str,
                        help='The input manifest.json source file to use.')
    parser.add_argument('--output', '-o', type=str, action="store", default=".",
                        help="The output directory to output %s." % RESULTS_JSON)
    args = parser.parse_args()

    try:
        if not os.path.exists(args.output):
            os.mkdir(args.output)
    except:
        print_error_and_exit("Unable to create directory %s." % str(args.output))

    return_code = SUCCESS
    err_msg = ""
    try:
        return_code, err_msg = produce_metrics(args.context, args.resources, args.phv, args.power,
                                               args.manifest, args.output)
    except:
        # import traceback
        # print(traceback.format_exc())
        print_error_and_exit("Unable to create %s." % RESULTS_JSON)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

#!/usr/bin/env python3

"""
This script produces a pa.characterize.log from an input phv.json file.
"""

import json, logging, math, os, sys
from collections import OrderedDict
from .utils import *

if not getattr(sys,'frozen', False):
    # standalone script
    MYPATH = os.path.dirname(__file__)
    SCHEMA_PATH = os.path.join(MYPATH, "../")
    sys.path.append(SCHEMA_PATH)

from schemas.schema_keys import *
from schemas.schema_enum_values import *

# The minimum phv.json schema version required.
MINIMUM_PHV_JSON_REQUIRED = "2.0.0"
RESULTS_FILE = "pa.characterize.log"
LOGGER_NAME = "PA-CHAR-RESULT-LOG"


# ----------------------------------------
# Helpers
# ----------------------------------------

class Record(object):
    def __init__(self, container_msb, container_lsb, field_name, field_msb, field_lsb,
                 field_class, field_gress):
        self.container_msb = container_msb
        self.container_lsb = container_lsb
        self.field_name = field_name
        self.field_msb = field_msb
        self.field_lsb = field_lsb
        self.field_class = field_class
        self.field_gress = field_gress
        if self.field_class == COMPILER_ADDED:  # FIXME: Added just for diffing purposes
            self.field_class = META

        # location to accsess type
        self.access = {}

    def add_access(self, location, access_type):
        if location in self.access:
            current_type = self.access[location]
            if current_type == READ_WRITE:
                pass
            elif current_type == READ and access_type == WRITE:
                self.access[location] = READ_WRITE
            elif current_type == WRITE and access_type == READ:
                self.access[location] = READ_WRITE
        else:
            self.access[location] = access_type

    def get_live_start_end(self, num_stages):
        # index list such that 0 = parser, stage + 1 = stage, num_stages = deparser

        live_start = None
        live_end = None

        if PARSER_ACCESS in self.access:
            live_start = 0
            live_end = 0
        if DEPARSER_ACCESS in self.access:
            if live_start is None:
                live_start = num_stages + 1
            live_end = num_stages + 1

        for stage in range(0, num_stages):
            if stage in self.access:
                if live_start is None:
                    live_start = stage + 1
                elif stage < live_start:
                    live_start = stage + 1
                if live_end is None:
                    live_end = stage + 1
                elif stage > live_end:
                    live_end = stage + 1

        # FIXME: Should this be an error?
        if live_start is None:
            live_start = 0
        if live_end is None:
            live_end = 0

        return live_start, live_end

    def log_record(self, num_stages):
        data = [" [%d:%d]" % (self.container_msb, self.container_lsb),
                self.field_gress,
                "%s[%d:%d]" % (self.field_name, self.field_msb, self.field_lsb),
                self.field_class]
        data.append("") # blank column
        stages = [""]*(num_stages + 2)
        live_start, live_end = self.get_live_start_end(num_stages)

        if live_start == 0:
            stages[0] = WRITE
        if live_end == (num_stages + 1):
            stages[-1] = READ

        for stage in range(0, num_stages):
            if stage in self.access:
                stages[stage + 1] = self.access[stage]

        for live in range(live_start, live_end + 1):
            if len(stages[live]) == 0:
                stages[live] = "~"

        data.extend(stages)
        return data


class Container(object):
    def __init__(self, address, bit_width, container_type, group_id):
        self.address = address
        self.bit_width = bit_width
        self.container_type = container_type
        self.group_id = group_id

        self.records = []

    def __str__(self):
        return "phv%d" % self.address

    def add_record(self, record):
        self.records.append(record)

    def get_gress(self):
        found = None
        for r in self.records:
            if found is None:
                found = r.field_gress
            elif found != r.field_gress:
                error_msg = "Unexpected mixed gress assignment for container %s." % str(self)
                print_error_and_exit(error_msg)
        if found is None:
            return "-"
        return found

    def get_container_class(self):
        is_overlay = False
        is_shared = len(self.records) > 1

        bits_used = {}  # bit number to cnt
        for b in range(0, self.bit_width):
            bits_used[b] = 0
        for r in self.records:
            for b in range(r.container_lsb, r.container_msb + 1):
                if b >= self.bit_width:
                    break
                bits_used[b] += 1

        for b in bits_used:
            if bits_used[b] > 1:
                is_overlay = True
                break

        if is_overlay and is_shared:
            return OVERLAY_AND_SHARE
        elif is_overlay:
            return OVERLAY
        elif is_shared:
            return SHARE
        else:
            return ""

    def log_container(self, num_stages):
        data = []
        blank_stages = [""]*(num_stages + 3)  # empty column, parser, stages, deparser

        c = ["phv%d" % self.address, self.get_gress(), "", self.get_container_class()]
        c.extend(blank_stages)
        data.append(c)

        # Sort such that records used in most significant part of container show up first in table.
        # Second criteria is field (slice) width, with smaller first.
        self.records.sort(key=lambda x: (-x.container_msb, (x.container_msb - x.container_lsb + 1)))
        for r in self.records:
            data.append(r.log_record(num_stages))

        return data


# ----------------------------------------
#  Produce log file
# ----------------------------------------

def _add_access(access, record, access_type):
    # table_name = get_attr(TABLE_NAME, access)
    location = get_attr(LOCATION, access)
    location_type = get_attr(TYPE, location)
    if location_type == "mau":
        location_stage = get_attr(STAGE, location)
        record.add_access(location_stage, access_type)
    else:
        record.add_access(location_type, access_type)

def _get_field_info(field_name, fields):
    for field in fields:
        field_info = get_attr(FIELD_INFO, field)
        if get_attr(FIELD_NAME, field_info) == field_name:
            return field_info

def _get_gress_info(phv_number, containers):
    for container in containers:
        if get_attr(PHV_NUMBER, container) == phv_number:
            return get_attr(GRESS, container)

def _parse_phv_json(context):

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_PHV_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported phv.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_PHV_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    num_stages = get_attr(NSTAGES, context)
    # address to Container
    all_containers = OrderedDict()

    # Set up all containers available
    resources = get_attr(RESOURCES, context)
    fields = get_attr(FIELDS, context)
    containers = get_attr(CONTAINERS, context)
    grp_on = 0
    for grp in resources:
        group_type = get_attr(GROUP_TYPE, grp)
        if group_type == DEPARSER:  # Not interested in deparser groups
            continue
        bit_width = get_attr(BIT_WIDTH, grp)
        addresses = get_attr(ADDRESSES, grp)

        for a in addresses:
            c = Container(a, bit_width, group_type, grp_on)
            all_containers[a] = c
        grp_on += 1

    containers = get_attr(CONTAINERS, context)
    for c in containers:
        phv_number = get_attr(PHV_NUMBER, c)

        if phv_number in all_containers:
            cobj = all_containers[phv_number]
            slices = get_attr(SLICES, c)
            for r in slices:
                field_slice = get_attr(FIELD_SLICE, r)
                field_name = get_attr(FIELD_NAME, field_slice)
                field_info = _get_field_info(field_name, fields)
                field_class = get_attr(FIELD_CLASS, field_info)
                field_slice_info = get_attr(SLICE_INFO, field_slice)
                field_lsb = get_attr(LSB, field_slice_info)
                field_msb = get_attr(MSB, field_slice_info)
                phv_slice_info = get_attr(SLICE_INFO, r)
                phv_lsb = get_attr(LSB, phv_slice_info)
                phv_msb = get_attr(MSB, phv_slice_info)
                phv_number = get_attr(PHV_NUMBER, r)
                gress = _get_gress_info(phv_number, containers)
                reads = get_attr(READS, r)
                writes = get_attr(WRITES, r)

                crec = Record(phv_msb, phv_lsb, field_name, field_msb, field_lsb, field_class, gress)
                for rd in reads:
                    _add_access(rd, crec, READ)
                for wr in writes:
                    _add_access(wr, crec, WRITE)

                cobj.add_record(crec)
        else:
            error_msg = "Container number '%s' was not specified as an address in %s node." % (str(phv_number), RESOURCES)
            print_error_and_exit(error_msg)

    return num_stages, all_containers


def log_stage_liveness(num_stages, all_containers):
    log = logging.getLogger(LOGGER_NAME)

    hdrs = [["Direction", "Location", "Fields Live", "Bits Live", "Header Fields Live", "Header Bits Live", "Metadata Fields Live", "Metadata Bits Live"]]
    data = []
    blank = [""] * len(hdrs[0])

    fields_saw = set()  # (field name, gress, location)
    total_hdr_saw = OrderedDict()
    total_meta_saw = OrderedDict()

    fields_live = OrderedDict()
    total_bits_live = OrderedDict()
    hdrs_live = OrderedDict()
    hdr_bits_live = OrderedDict()
    meta_live = OrderedDict()
    meta_bits_live = OrderedDict()
    for g in [INGRESS, EGRESS]:
        fields_live[g] = OrderedDict()
        total_bits_live[g] = OrderedDict()
        hdrs_live[g] = OrderedDict()
        hdr_bits_live[g] = OrderedDict()
        meta_bits_live[g] = OrderedDict()
        meta_live[g] = OrderedDict()
        total_hdr_saw[g] = set()
        total_meta_saw[g] = set()
        for s in range(0, num_stages + 2):
            fields_live[g][s] = 0
            total_bits_live[g][s] = 0
            hdrs_live[g][s] = 0
            hdr_bits_live[g][s] = 0
            meta_bits_live[g][s] = 0
            meta_live[g][s] = 0

    for addr in all_containers:
        c = all_containers[addr]
        if len(c.records) > 0:
            gress = c.get_gress()
            if gress in fields_live:
                for r in c.records:
                    live_start, live_end = r.get_live_start_end(num_stages)
                    for loc in range(live_start, live_end + 1):
                        if (r.field_name, r.field_gress, loc) not in fields_saw:
                            fields_live[gress][loc] += 1
                            if r.field_class == PKT:
                                hdrs_live[gress][loc] += 1
                            else:
                                meta_live[gress][loc] += 1

                        total_bits_live[gress][loc] += (r.field_msb - r.field_lsb + 1)
                        if r.field_class == PKT:
                            hdr_bits_live[gress][loc] += (r.field_msb - r.field_lsb + 1)
                            total_hdr_saw[gress].add(r.field_name)
                        else:
                            meta_bits_live[gress][loc] += (r.field_msb - r.field_lsb + 1)
                            total_meta_saw[gress].add(r.field_name)

                        fields_saw.add((r.field_name, r.field_gress, loc))

    for g in [INGRESS, EGRESS]:
        total_hdr = len(total_hdr_saw[g])
        total_meta = len(total_meta_saw[g])
        total_fields = total_hdr + total_meta

        for loc in fields_live[g]:
            actual_loc = "Stage %d" % (loc - 1)
            if loc == 0:
                actual_loc = "Parser"
            elif loc == (num_stages + 1):
                actual_loc = "Deparser"

            fields_pcent = 0
            if total_fields > 0:
                fields_pcent = 100.0 * float(fields_live[g][loc]) / float(total_fields)
            hdr_pcent = 0
            if total_hdr > 0:
                hdr_pcent = 100.0 * float(hdrs_live[g][loc]) / float(total_hdr)
            meta_pcent = 0
            if total_meta > 0:
                meta_pcent = 100.0 * float(meta_live[g][loc]) / float(total_meta)

            data.append([g, actual_loc,
                         "%d (%.2f%%)" % (fields_live[g][loc], fields_pcent), total_bits_live[g][loc],
                         "%d (%.2f%%)" % (hdrs_live[g][loc], hdr_pcent), hdr_bits_live[g][loc],
                         "%d (%.2f%%)" % (meta_live[g][loc], meta_pcent), meta_bits_live[g][loc]])

        if g == INGRESS:
            data.append(blank)

    log.info("")
    log.info("")
    log.info(print_table(hdrs, data))


def log_container_liveness(num_stages, all_containers):
    log = logging.getLogger(LOGGER_NAME)

    hdrs = ["Direction", "Container", "Parser"]
    for s in range(0, num_stages):
        hdrs.append("Stage %d" % s)
    hdrs.append("Deparser")
    hdrs = [hdrs]
    data = []
    blank = [""] * len(hdrs[0])

    addr_keys = list(all_containers.keys())
    addr_keys.sort()

    last_group_id = None

    for addr in addr_keys:
        c = all_containers[addr]
        if len(c.records) > 0:
            bits_used = OrderedDict()
            fields_live = OrderedDict()
            for loc in range(0, num_stages + 2):
                bits_used[loc] = set()
                fields_live[loc] = set()

            for r in c.records:
                live_start, live_end = r.get_live_start_end(num_stages)
                for loc in range(live_start, live_end + 1):
                    fields_live[loc].add(r.field_name)
                    for b in range(r.container_lsb, r.container_msb + 1):
                        bits_used[loc].add(b)

            line = [c.get_gress(), "phv%d" % c.address]
            for loc in fields_live:
                if len(fields_live[loc]) > 0:
                    line.append("%d (%s bits)" % (len(fields_live[loc]), len(bits_used[loc])))
                else:
                    line.append("-")

            if last_group_id is not None and last_group_id != c.group_id:
                data.append(blank)
            data.append(line)
            last_group_id = c.group_id

    log.info("")
    log.info("")
    log.info("")
    log.info(print_table(hdrs, data))


def produce_pa_characterize(source, output):
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
        context = {}
        json_file.close()
        print_error_and_exit("Unable to open JSON file '%s'" % str(source))

    compiler_version = get_attr(COMPILER_VERSION, context)
    build_date = get_attr(BUILD_DATE, context)
    run_id = get_attr(RUN_ID, context)
    program_name = get_attr(PROGRAM_NAME, context)

    box = "+---------------------------------------------------------------------+"
    log.info(box)
    for s in ["Log file: %s" % RESULTS_FILE,
              "Compiler version: %s" % str(compiler_version),
              "Created on: %s" % str(build_date),
              "Run ID: %s" % str(run_id)]:
        line = "|  %s" % s
        while len(line) < (len(box) - 1):
            line += " "
        line += "|"
        log.info(line)
    log.info("%s\n" % box)
    log.info("Program: %s\n" % program_name)

    # Find container usage
    num_stages, all_containers = _parse_phv_json(context)

    # Output summary table in log file
    hdrs = ["Container", "Gress", "Name", "Class", "", "P"]
    stages = []
    for s in range(0, num_stages):
        stages.append("%d" % s)
    hdrs.extend(stages)
    hdrs.append("D")
    hdrs = [hdrs]
    blank = [""]*len(hdrs[0])

    data = []
    addr_keys = list(all_containers.keys())
    addr_keys.sort()

    last_grp_id = None
    containers_used = 0
    containers_with_overlay = 0
    containers_with_share = 0

    for addr in addr_keys:
        c = all_containers[addr]

        if last_grp_id is None:
            pass
        elif last_grp_id != c.group_id:
            data.append(blank)

        data.extend(c.log_container(num_stages))
        last_grp_id = c.group_id
        if len(c.records) > 0:
            containers_used += 1
        container_class = c.get_container_class()
        if container_class == OVERLAY_AND_SHARE:
            containers_with_overlay += 1
            containers_with_share += 1
        elif container_class == OVERLAY:
            containers_with_overlay += 1
        elif container_class == SHARE:
            containers_with_share += 1

    log.info(print_table(hdrs, data))

    pcent_overlay = 0
    pcent_share = 0
    if containers_used > 0:
        pcent_overlay = 100.0 * float(containers_with_overlay) / float(containers_used)
        pcent_share = 100.0 * float(containers_with_share) / float(containers_used)

    log.info("\nContainers used: %d" % containers_used)
    log.info("Containers with data overlayed: %d  (%.2f%%)" % (containers_with_overlay, pcent_overlay))
    log.info("Containers shared: %d  (%.2f%%)" % (containers_with_share, pcent_share))

    log.info("\n------------------------")
    log.info("  Legend:")
    log.info("------------------------")
    log.info("   P:     Parsed")
    log.info("   D:     Deparsed")
    log.info("   OL:    Overlay")
    log.info("   SH:    Shared")
    log.info("   pkt:   Packet data")
    log.info("   meta:  Metadata")
    log.info("   imeta: Intrinsic Metadata")
    log.info("   pov:   Packet Occupancy Vector bit")
    log.info("   R:     Read")
    log.info("   W:     Write")
    log.info("   ~:     Field is live")

    log_stage_liveness(num_stages, all_containers)
    log_container_liveness(num_stages, all_containers)

    # Tidy up
    json_file.close()
    return SUCCESS, "ok"


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument('source', metavar='source', type=str,
                        help='The input phv.json source file to use.')
    parser.add_argument('--output', '-o', type=str, action="store", default=".",
                        help="The output directory to output %s." % RESULTS_FILE)
    args = parser.parse_args()

    try:
        if not os.path.exists(args.output):
            os.mkdir(args.output)
    except:
        print_error_and_exit("Unable to create directory %s." % str(args.output))

    return_code = SUCCESS
    err_msg = ""
    try:
        return_code, err_msg = produce_pa_characterize(args.source, args.output)
    except:
        raise
        print_error_and_exit("Unable to create %s." % RESULTS_FILE)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

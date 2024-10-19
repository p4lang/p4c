#!/usr/bin/env python3

"""
This script produces a pa.results.log from an input context.json file.
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

# The minimum context.json schema version required.
MINIMUM_CONTEXT_JSON_REQUIRED = "1.7.0"
# The minimum phv.json schema version required.
MINIMUM_PHV_JSON_REQUIRED = "2.0.0"
RESULTS_FILE = "pa.results.log"
LOGGER_NAME = "PA-RESULT-LOG"
CONTAINER_TYPE_MAP = {NORMAL: "n", MOCHA: "m", DARK: "d", TAGALONG: "t", "": ""}


class Record(object):
    def __init__(
        self,
        container_size,
        address,
        gress,
        container_msb,
        container_lsb,
        field_name,
        field_msb,
        field_lsb,
        is_deparsed,
        container_type="",
    ):
        self.container_size = container_size
        self.address = address
        self.gress = gress
        self.container_type = container_type

        self.container_msb = container_msb
        self.container_lsb = container_lsb

        self.field_name = field_name
        self.field_msb = field_msb
        self.field_lsb = field_lsb
        self.is_deparsed = is_deparsed

    def __str__(self):
        d = ""
        if self.is_deparsed:
            d = " (deparsed)"
        t = ""
        if self.address >= 256:
            t = " (tagalong capable)"
        ct = CONTAINER_TYPE_MAP[self.container_type]

        return "%d-bit PHV %d%s (%s): phv%d[%d:%d] = %s[%d:%d]%s%s" % (
            self.container_size,
            self.address,
            ct,
            self.gress,
            self.address,
            self.container_msb,
            self.container_lsb,
            self.field_name,
            self.field_msb,
            self.field_lsb,
            t,
            d,
        )


# FIXME: This doesn't account for --extra-phv
def addr_to_bw(addr, target=TARGET.TOFINO):
    if target == TARGET.TOFINO or target is None:
        if 0 <= addr < 64:
            return 32
        elif 64 <= addr < 128:
            return 8
        elif 128 <= addr < 224:
            return 16
        elif 256 <= addr < 288:
            return 32
        elif 288 <= addr < 320:
            return 8
        elif addr == 368:
            return 0
        else:
            return 16
    elif target != TARGET.TOFINO:
        if 0 <= addr < 80:
            return 32
        elif 80 <= addr < 160:
            return 8
        elif addr == 280:  # Used for loop below
            return 0
        else:
            return 16
    else:
        print_error_and_exit("Unknown target '%s'" % str(target))


def _get_data(text, containers_used, containers_available, bits_used, bits_available):
    cutil = 0
    if containers_available != 0:
        cutil = 100.0 * float(containers_used) / float(containers_available)
    butil = 0
    if bits_available != 0:
        butil = 100.0 * float(bits_used) / float(bits_available)
    return [
        str(text),
        "%d (%.2f%%)" % (containers_used, cutil),
        "%d (%.2f%%)" % (bits_used, butil),
        "%d" % bits_available,
    ]


# ----------------------------------------
#  Produce log file
# ----------------------------------------


def _parse_context_json(context):
    # Container number to list of PHV allocation records
    container_to_records = OrderedDict()
    # gress to set of (index, container, pov name)
    pov_gress = {INGRESS: set(), EGRESS: set()}
    # (name, gress) to bit width
    field_widths = OrderedDict()

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_CONTEXT_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported context.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_CONTEXT_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    target = get_attr_first_available(TARGET_KEY, context, "1.6.4", schema_version)

    phv_alloc = get_attr(PHV_ALLOCATION, context)
    for phv_alloc_stage in phv_alloc:
        stage = get_attr(STAGE_NUMBER, phv_alloc_stage)

        for g in [INGRESS, EGRESS]:
            gress_allocs = get_attr(g, phv_alloc_stage)

            for phv_dict in gress_allocs:
                phv_num = get_attr(PHV_NUMBER, phv_dict)
                ctype = get_attr(CONTAINER_TYPE, phv_dict)
                csize = get_attr(WORD_BIT_WIDTH, phv_dict)
                records = get_attr(RECORDS, phv_dict)

                if phv_num not in container_to_records:
                    container_to_records[phv_num] = []

                for r in records:
                    is_pov = get_attr(IS_POV, r)

                    fname = get_attr(FIELD_NAME, r)
                    flsb = get_attr(LSB, r)
                    fmsb = get_attr(MSB, r)
                    if (fname, g) not in field_widths:
                        field_widths[(fname, g)] = 0
                    field_widths[(fname, g)] += fmsb - flsb + 1

                    phv_lsb = get_attr(PHV_LSB, r)
                    phv_msb = get_attr(PHV_MSB, r)

                    live_end = None
                    if has_attr(LIVE_END, r):
                        live_end = get_attr(LIVE_END, r)

                    ct = ""
                    if target != TARGET.TOFINO:
                        ct = ctype

                    if is_pov:
                        pov_headers = get_attr(POV_HEADERS, r)
                        for pov in pov_headers:
                            hdr_name = get_attr(HEADER_NAME, pov)
                            n = "%s%s" % (VALIDITY_CHECK, hdr_name)
                            bit_index = get_attr(BIT_INDEX, pov)
                            rec = Record(csize, phv_num, g, bit_index, bit_index, n, 0, 0, True, ct)
                            container_to_records[phv_num].append(rec)
                            pov_gress[g].add((bit_index, phv_num, csize, n))
                    else:
                        rec = Record(
                            csize,
                            phv_num,
                            g,
                            phv_msb,
                            phv_lsb,
                            fname,
                            fmsb,
                            flsb,
                            live_end == LIVE_DEPARSER,
                            ct,
                        )
                        container_to_records[phv_num].append(rec)

        break  # Only care about stage 0 for logging purposes (currently)

    return container_to_records, pov_gress, field_widths


# TODO: Move this to a common file to avoid duplication
def _get_field_info(field_name, fields):
    for field in fields:
        field_info = get_attr(FIELD_INFO, field)
        if get_attr(FIELD_NAME, field_info) == field_name:
            return field_info


def _parse_phv_json(context):
    # Container number to list of PHV allocation records
    container_to_records = OrderedDict()
    # gress to set of (index, container, container bit width, pov name)
    pov_gress = {INGRESS: set(), EGRESS: set()}
    # (name, gress) to bit width
    field_widths = OrderedDict()

    field_gress_to_container_size = {}

    schema_version = get_attr(SCHEMA_VERSION, context)
    if not version_check_ge(MINIMUM_PHV_JSON_REQUIRED, schema_version):
        error_msg = "Unsupported phv.json schema version %s.\n" % str(schema_version)
        error_msg += "Minimum required version is: %s" % str(MINIMUM_PHV_JSON_REQUIRED)
        print_error_and_exit(error_msg)

    # Older versions will not know correct container types
    target = get_attr_first_available(TARGET_KEY, context, "1.0.3", schema_version)

    containers = get_attr(CONTAINERS, context)
    fields = get_attr(FIELDS, context)
    for c in containers:
        phv_num = get_attr(PHV_NUMBER, c)
        bit_width = get_attr(BIT_WIDTH, c)
        slices = get_attr(SLICES, c)
        ctype = get_attr(CONTAINER_TYPE, c)
        g = get_attr(GRESS, c)

        ct = ""
        if target != TARGET.TOFINO:
            ct = ctype

        for r in slices:
            if phv_num not in container_to_records:
                container_to_records[phv_num] = []

            fslice = get_attr(FIELD_SLICE, r)
            fname = get_attr(FIELD_NAME, fslice)
            fslinfo = get_attr(SLICE_INFO, fslice)
            flsb = get_attr(LSB, fslinfo)
            fmsb = get_attr(MSB, fslinfo)
            pslinfo = get_attr(SLICE_INFO, r)
            phv_lsb = get_attr(LSB, pslinfo)
            phv_msb = get_attr(MSB, pslinfo)
            reads = get_attr(READS, r)

            if (fname, g) not in field_widths:
                field_widths[(fname, g)] = 0
            field_widths[(fname, g)] += fmsb - flsb + 1

            is_deparsed = False
            for access in reads:
                loc = get_attr(LOCATION, access)
                loctype = get_attr(TYPE, loc)
                if loctype == LIVE_DEPARSER:
                    is_deparsed = True
                    break

            rec = Record(
                bit_width, phv_num, g, phv_msb, phv_lsb, fname, fmsb, flsb, is_deparsed, ct
            )
            field_gress_to_container_size[(fname, g)] = (phv_num, bit_width)
            container_to_records[phv_num].append(rec)

    pov_structure = get_attr(POV_STRUCTURE, context)
    for pov in pov_structure:
        pov_bit_index = get_attr(POV_BIT_INDEX, pov)
        pov_bit_name = get_attr(POV_BIT_NAME, pov)
        g = get_attr(GRESS, pov)
        phv_num, phv_bw = field_gress_to_container_size[(pov_bit_name, g)]
        pov_gress[g].add((pov_bit_index, phv_num, phv_bw, pov_bit_name))

    return container_to_records, pov_gress, field_widths


def produce_pa_results(source, output):
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
        context = {}
        print_error_and_exit("Unable to open JSON file '%s'" % str(source))

    compiler_version = get_attr(COMPILER_VERSION, context)
    schema_version = get_attr(SCHEMA_VERSION, context)
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
    # Determine if user passed in context.json or phv.json
    using_context_json = False
    if has_attr(PHV_ALLOCATION, context):
        container_to_records, pov_gress, field_widths = _parse_context_json(context)
        target = get_attr_first_available(TARGET_KEY, context, "1.6.4", schema_version)
        using_context_json = True
    elif has_attr(POV_STRUCTURE, context):
        container_to_records, pov_gress, field_widths = _parse_phv_json(context)
        target = get_attr_first_available(TARGET_KEY, context, "1.0.3", schema_version)
    else:
        container_to_records = {}
        target = None
        pov_gress = {}
        field_widths = {}
        error_msg = "Unknown JSON format in %s." % str(source)
        print_error_and_exit(error_msg)

    # Output summary table in log file
    hdrs = [
        ["PHV Group", "Containers Used", "Bits Used", "Bits Available"],
        ["(container bit widths)", "(% used)", "(% used)", ""],
    ]
    data = []

    mau_grp_containers = 16
    if target == TARGET.TOFINO or target is None:
        addresses = list(range(0, 224))
        addresses.extend(list(range(256, 369)))  # One beyond so can just use loop below
    elif target is not None and target != TARGET.TOFINO:
        addresses = list(range(0, 281))  # One beyond so can just use loop below
        mau_grp_containers = 20
    else:
        addresses = []
        print_error_and_exit("Unknown target '%s'" % str(target))

    grp_bw = 32
    grp_num = 0
    grp_avail = 0
    grp_container_to_bits = {}
    is_tag = False

    total_grp_bits = 0
    total_grp_containers = 0
    total_grp_avail = 0

    mau_total_grp_bits = 0
    mau_total_grp_containers = 0
    mau_total_grp_avail = 0
    tag_total_grp_bits = 0
    tag_total_grp_containers = 0
    tag_total_grp_avail = 0

    for addr in addresses:
        if addr != 0 and (addr % mau_grp_containers) == 0:
            cused = len(grp_container_to_bits)
            bavail = grp_avail * grp_bw
            bused = 0
            for a in grp_container_to_bits:
                bused += len(grp_container_to_bits[a])
                # print "bits used in addr %d is %d" % (a, len(grp_container_to_bits[a]))

            total_grp_bits += bused
            total_grp_containers += cused
            total_grp_avail += grp_avail

            t = ""
            if is_tag:
                t = " T"

            text = "%d (%d)%s" % (grp_num, grp_bw, t)
            data.append(_get_data(text, cused, grp_avail, bused, bavail))

            next_grp_bw = addr_to_bw(addr, target)
            if next_grp_bw != grp_bw:
                bavail = total_grp_avail * grp_bw
                text = "Total for %d bit" % grp_bw
                data.append(
                    _get_data(text, total_grp_containers, total_grp_avail, total_grp_bits, bavail)
                )
                data.append(["", "", "", ""])

                if is_tag:
                    tag_total_grp_bits += total_grp_bits
                    tag_total_grp_containers += total_grp_containers
                    tag_total_grp_avail += total_grp_avail
                else:
                    mau_total_grp_bits += total_grp_bits
                    mau_total_grp_containers += total_grp_containers
                    mau_total_grp_avail += total_grp_avail

                total_grp_bits = 0
                total_grp_containers = 0
                total_grp_avail = 0

            grp_bw = addr_to_bw(addr, target)
            grp_num += 1
            grp_avail = 0
            grp_container_to_bits = {}
            if addr >= 256 and target == TARGET.TOFINO:
                is_tag = True

        grp_avail += 1
        if addr in container_to_records:
            grp_container_to_bits[addr] = set()
            for rec in container_to_records[addr]:
                for b in range(rec.container_lsb, rec.container_msb + 1):
                    grp_container_to_bits[addr].add(b)

    if target == TARGET.TOFINO:
        text = "MAU total"
        data.append(
            _get_data(text, mau_total_grp_containers, mau_total_grp_avail, mau_total_grp_bits, 4096)
        )
        text = "Tagalong total"
        data.append(
            _get_data(text, tag_total_grp_containers, tag_total_grp_avail, tag_total_grp_bits, 2048)
        )
        text = "Overall total"
        data.append(
            _get_data(
                text,
                mau_total_grp_containers + tag_total_grp_containers,
                mau_total_grp_avail + tag_total_grp_avail,
                mau_total_grp_bits + tag_total_grp_bits,
                4096 + 2048,
            )
        )
    elif target is not None and target != TARGET.TOFINO:
        text = "Overall total"
        data.append(
            _get_data(text, mau_total_grp_containers, mau_total_grp_avail, mau_total_grp_bits, 5120)
        )
    elif target is not None:
        print_error_and_exit("Unknown target '%s'" % str(target))

    log.info("Allocation state: Final Allocation")
    log.info(print_table(hdrs, data))

    # Output allocations

    log.info("--------------------------------------------")
    log.info("PHV Allocation")
    log.info("--------------------------------------------\n")

    grp_ingress = 0
    grp_egress = 0
    grp_cnt = 0
    grp_num = 0

    for addr in addresses:
        if addr != 0 and (addr % mau_grp_containers) == 0:
            if grp_cnt > 0:
                log.info("  >> %d in ingress and %d in egress\n" % (grp_ingress, grp_egress))
            grp_ingress = 0
            grp_egress = 0
            grp_cnt = 0
            grp_num += 1

        if addr in container_to_records:
            if grp_cnt == 0:
                t = ""
                if addr >= 256 and target == TARGET.TOFINO:
                    t = " (tagalong)"
                log.info(
                    "Allocations in Group %d %d bits%s" % (grp_num, addr_to_bw(addr, target), t)
                )
            grp_cnt += 1

            ing = True
            container_to_records[addr].sort(key=lambda x: -x.container_msb)
            for rec in container_to_records[addr]:
                if rec.gress == "egress":
                    ing = False
                log.info("  %s" % str(rec))

            if ing:
                grp_ingress += 1
            else:
                grp_egress += 1

    for g in [INGRESS, EGRESS]:
        log.info("\nFinal POV layout (%s):" % g)
        povs = list(pov_gress[g])
        povs.sort(key=lambda x: (x[1], x[0]))
        coff = {}
        off = 0
        for b, c, csz, n in povs:
            if c not in coff:
                coff[c] = off
                off += csz
            if using_context_json:
                log.info("%3d: %s (%s) in container %d" % (coff[c] + b, n, g, c))
            else:
                log.info("%3d: %s (%s) in container %d" % (b, n, g, c))

    if not using_context_json:
        structures = get_attr(STRUCTURES, context)
        bridge = None
        phase0 = None

        for s in structures:
            the_type = get_attr(TYPE, s)
            ordered_fields = get_attr(ORDERED_FIELDS, s)
            if the_type == BRIDGE:
                bridge = ordered_fields
            elif the_type == PHASE0:
                phase0 = ordered_fields

        def _get_width(fields):
            bits = 0
            for f in fields:
                if (f, INGRESS) in field_widths:
                    bits += field_widths[f, INGRESS]
            return (bits + 7) // 8

        def _print_fields(fields):
            for f in fields:
                w = ""
                if (f, INGRESS) in field_widths:
                    w = "[%d:%d]" % (field_widths[f, INGRESS] - 1, 0)
                log.info("  %s%s" % (f, w))

        if bridge is not None:
            log.info("\n--------------------------------------------")
            log.info("   Bridged metadata layout (%d bytes)" % _get_width(bridge))
            log.info("--------------------------------------------")

            log.info("Final ingress layout:")
            _print_fields(bridge)
            log.info("\nFinal egress layout:")
            _print_fields(bridge)

        if phase0 is not None:
            log.info("\n--------------------------------------------")
            log.info("   Phase 0 metadata layout (%d bytes)" % _get_width(phase0))
            log.info("--------------------------------------------")
            _print_fields(phase0)

    # Tidy up
    json_file.close()

    return SUCCESS, "ok"


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument(
        'source',
        metavar='source',
        type=str,
        help='The input context.json or phv.json source file to use.',
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
        return_code, err_msg = produce_pa_results(args.source, args.output)
    except:
        print_error_and_exit("Unable to create %s." % RESULTS_FILE)

    if return_code == FAILURE:
        print_error_and_exit(err_msg)
    sys.exit(SUCCESS)

#!/usr/bin/env python3

"""
This file holds the various JSON attribute enum values that are used by the
tools.  Rather than duplicating definitions, let's put them in a common file.
"""

import os.path
import sys

MYPATH = os.path.dirname(__file__)
if not getattr(sys,'frozen', False):
    # standalone script
    SCHEMA_PATH = os.path.join(MYPATH, "../")
    sys.path.append(SCHEMA_PATH)

if os.path.exists(os.path.join(MYPATH, 'targets.py')):
    from schemas.targets import TARGETS
else:
    TARGETS=['tofino', 'tofino2', 'tofino2h', 'tofino2m', 'tofino2u']

# ----------------------------------------
# MAU Schema Enum Values
# ----------------------------------------
BUF = "buf"
PARITY = "parity"
PAYLOAD = "payload"

ACTION_INSTR = "action_instr"
ACTION_DATA_ADDR = "action_data_addr"
METER_ADDR = "meter_addr"
STATEFUL_ADDR = "stateful_addr"
SELECTOR_ADDR = "selector_addr"
SELECTOR_LENGTH = "selector_length"
SELECTOR_SHIFT = "selector_shift"
STATISTICS_ADDR = "statistics_addr"

ACTION = "action"
MATCH = "match"
METER = "meter"
SELECTION = "selection"
SRAM = "sram"
STATEFUL = "stateful"
STATISTICS = "statistics"
TCAM = "tcam"
TIND = "tind"

OH_VERSION = "version"
OH_VERSION_VALID = "version/valid"
OH_INSTR = "action_instr"
OH_ADT_PTR = "action_data_addr"
OH_METER_PTR = "meter_addr"
OH_SEL_PTR = "selector_addr"
OH_STATS_PTR = "statistics_addr"
OH_STFUL_PTR = "stateful_addr"
OH_NEXT_TABLE = "next_table"
OH_SELECTION_LENGTH = "selector_length"
OH_SELECTION_LENGTH_SHIFT = "selector_shift"
OH_IMMEDIATE = "immediate"

# Do not include version/valid in this list
ALL_OH = [OH_VERSION, OH_INSTR, OH_ADT_PTR, OH_METER_PTR, OH_SEL_PTR, OH_STATS_PTR, OH_STFUL_PTR,
          OH_NEXT_TABLE, OH_SELECTION_LENGTH, OH_SELECTION_LENGTH_SHIFT, OH_IMMEDIATE]

OVERHEAD_SOURCE_TO_NAME = {OH_VERSION: OH_VERSION,
                           OH_VERSION_VALID: OH_VERSION_VALID,
                           OH_INSTR: ACTION_INSTR,
                           OH_ADT_PTR: ACTION_DATA_ADDR,
                           OH_METER_PTR: METER_ADDR,
                           OH_SEL_PTR: SELECTOR_ADDR,
                           OH_STATS_PTR: STATISTICS_ADDR,
                           OH_STFUL_PTR: STATEFUL_ADDR,
                           OH_NEXT_TABLE: OH_NEXT_TABLE,
                           OH_SELECTION_LENGTH: SELECTOR_LENGTH,
                           OH_SELECTION_LENGTH_SHIFT: SELECTOR_SHIFT,
                           OH_IMMEDIATE: OH_IMMEDIATE}


# ----------------------------------------
# Context Schema Enum Values
# ----------------------------------------

INGRESS_BUFFER = "ingress_buffer"

ALGORITHMIC_LPM = "algorithmic_lpm"
ALGORITHMIC_TCAM = "algorithmic_tcam"
ALGORITHMIC_TCAM_MATCH = "algorithmic_tcam_match"
CHAINED_LPM = "chained_lpm"
CONDITION = "condition"
DIRECT_MATCH = "direct_match"
HASH_ACTION = "hash_action"
HASH_MATCH = "hash_match"
HASH_WAY = "hash_way"
MATCH_WITH_NO_KEY = "match_with_no_key"
PHASE_0_MATCH = "phase_0_match"
PHV = "phv"
PROXY_HASH = "proxy_hash"
PROXY_HASH_MATCH = "proxy_hash_match"
SPEC = "spec"
TERNARY_MATCH = "ternary_match"

# Create TARGET.TOFINO, TARGET.TOFINO2, etc.
__tdict = {}
for t in TARGETS:
    __tdict[t.upper()] = t
TARGET = type('Enum', (), __tdict)

ZERO = "zero"
EXACT = "exact"
INGRESS = "ingress"
GHOST = "ghost"
HASH_PARITY_BIT = "<hash parity bit>"
TERNARY = "ternary"
UNUSED = "--UNUSED--"


# ----------------------------------------
# Phv Schema Enum Values
# ----------------------------------------
BRIDGE = "bridge"
COMPILER_ADDED = "compiler-added"
DARK = "dark"
DEPARSER = "deparser"
DEPARSER_ACCESS = "deparser"
LIVE_DEPARSER = "deparser"
LIVE_PARSER = "parser"
META = "meta"
MOCHA = "mocha"
NORMAL = "normal"
OVERLAY = "OL"
OVERLAY_AND_SHARE = "OL,SH"
PARSER_ACCESS = "parser"
PHASE0 = "phase0"
PKT = "pkt"
READ = "R"
READ_WRITE = "RW"
SHARE = "SH"
TAGALONG = "tagalong"
VALIDITY_CHECK = "--validity_check--"
VLIW = "vliw"
WRITE = "W"
XBAR = "xbar"

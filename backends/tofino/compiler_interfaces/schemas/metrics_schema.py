#!/usr/bin/env python3

"""
metrics_schema.py: Generates a JSON Schema model for structured compilation metrics.
"""

import jsl
import json
import inspect
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

########################################################
#   Schema Version
########################################################

"""
Version Notes:

1.0.0:
- Initial version.

1.1.0:
- Added a new 'clots' node for CLOT-related metrics, with the following subnodes:
    - all_allocated_bits - the total number of bits allocated to CLOTs,
      including those overwritten by PHVs and checksums (i.e., including used and checksum bits)
    - allocated_bits - the number of unused, non-checksum bits that are CLOT-allocated
    - unallocated_bits - the number of unused, non-checksum bits in
      CLOT-eligible fields that are not CLOT-allocated
    - redundant_phv_bits - the number of unused, non-checksum bits in
      CLOT-eligible fields that are double-allocated to CLOTs and PHVs

  Here, "used" and "unused" refer to usage (or non-usage) by the MAU pipeline.
"""

major_version = 1
medium_version = 1
minor_version = 0

def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(medium_version), str(minor_version))

########################################################

# ------------------------------
#  PHV
# ------------------------------

class PhvTypeUsage(jsl.Document):
    title = "PhvTypeUsage"
    description = "Phv resource usage information for a specific type of container."
    bit_width = jsl.IntField(required=True, description="The bit width of this container type")
    containers_occupied = jsl.IntField(required=True, description="Total number of containers occupied.")
    bits_occupied = jsl.IntField(required=True, description="Total number of bits occupied.")
    bits_overlaid = jsl.IntField(required=True, description="Total number of bits overlaid.")
    bits_unusable = jsl.IntField(required=False, description="Total number of container bits that cannot be used due to allocation constraints.")


class Phv(jsl.Document):
    title = "Phv"
    description = "Phv resource usage information."
    normal =jsl.ArrayField(items=jsl.DocumentField("PhvTypeUsage", as_ref=True), required=True, description="Normal PHV containers.")
    tagalong = jsl.ArrayField(items=jsl.DocumentField("PhvTypeUsage", as_ref=True), required=False, description="Tagalong PHV containers.")
    mocha = jsl.ArrayField(items=jsl.DocumentField("PhvTypeUsage", as_ref=True), required=False, description="Mocha PHV containers.")
    dark = jsl.ArrayField(items=jsl.DocumentField("PhvTypeUsage", as_ref=True), required=False, description="Dark PHV containers.")


# ------------------------------
#  Parser
# ------------------------------

class GressParser(jsl.Document):
    title = "GressParser"
    description = "Per-gress parser resource usage information."
    tcam_rows = jsl.IntField(required=True, description="Total number of TCAM rows used.")

class Parser(jsl.Document):
    title = "Parser"
    description = "Parser resource usage information."
    ingress = jsl.DocumentField("GressParser", as_ref=True, required=True, description="Ingress parser resource information.")
    egress = jsl.DocumentField("GressParser", as_ref=True, required=True, description="Egress parser resource information.")


# ------------------------------
#  MAU
# ------------------------------

class Mau(jsl.Document):
    title = "MAU"
    description = "MAU resource usage information."
    srams = jsl.IntField(required=True, description="Total number of SRAMs used.")
    tcams = jsl.IntField(required=True, description="Total number of TCAMs used.")
    map_rams = jsl.IntField(required=True, description="Total number of MapRAMs used.")
    logical_tables = jsl.IntField(required=True, description="Total number of logical tables used.")
    action_bus_bytes = jsl.IntField(required=True, description="Total number of action data bus bytes used.")
    exact_crossbar_bytes = jsl.IntField(required=True, description="Total number of exact match crossbar bytes used.")
    ternary_crossbar_bytes = jsl.IntField(required=True, description="Total number of ternary match crossbar bytes used.")
    latency = jsl.ArrayField(items=jsl.DictField(properties={
        "gress": jsl.StringField(required=True, enum=["ingress", "egress", "ghost"], description="The gress this latency measurement applies to."),
        "cycles": jsl.IntField(required=True, description="The total number of clock cycles in this gress.")
    }, required=True), required=True, description="MAU power information.")

    power = jsl.ArrayField(items=jsl.DictField(properties={
        "gress": jsl.StringField(required=True, enum=["ingress", "egress", "ghost"], description="The gress this power estimate applies to."),
        "estimate": jsl.StringField(required=True, description="The estimated worst case table control flow power, in Watts.")
    }, required=True), required=True, description="MAU power information.")

# ------------------------------
#  Deparser
# ------------------------------

class GressDeparser(jsl.Document):
    title = "GressDeparser"
    description = "Per-gress deparser resource usage information."
    pov_bits = jsl.IntField(required=True, description="Total number of POV bits used.")
    fde_entries = jsl.IntField(required=True, description="Total number of field dictionary entries used.")

class Deparser(jsl.Document):
    title = "Deparser"
    description = "Deparser resource usage information."
    ingress = jsl.DocumentField("GressDeparser", as_ref=True, required=True, description="Ingress deparser resource information.")
    egress = jsl.DocumentField("GressDeparser", as_ref=True, required=True, description="Egress deparser resource information.")


# ------------------------------
#  CLOTs
# ------------------------------

class GressClots(jsl.Document):
    title="GressClots"
    description = "Per-gress information on the effectiveness of CLOT usage."
    all_allocated_bits = jsl.IntField(required=True, description="Total number of bits allocated to CLOTs, including those overwritten by PHVs and checksums (i.e., including bits used by MAU and checksum bits).")
    allocated_bits = jsl.IntField(required=True, description="Number of unused (by MAU), non-checksum bits that are CLOT-allocated.")
    unallocated_bits = jsl.IntField(required=True, description="Number of unused (by MAU), non-checksum bits in CLOT-eligible fields that are not CLOT-allocated.")
    redundant_phv_bits = jsl.IntField(required=True, description="Number of unused (by MAU), non-checksum bits in CLOT-eligible fields that are double-allocated to CLOTs and PHVs.")

class Clots(jsl.Document):
    title="clot"
    description = "Information on the effectiveness of CLOT usage."
    ingress = jsl.DocumentField("GressClots", as_ref=True, required=True, description="Ingress CLOT usage information.")
    egress = jsl.DocumentField("GressClots", as_ref=True, required=True, description="Egress CLOT usage information.")


# ------------------------------
#  Top level JSON schema
# ------------------------------

class MetricsJSONSchema(jsl.Document):
    title = "MetricsJSONSchema"
    description = "Metrics JSON schema for a P4 program's compilation results."
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    build_date = jsl.StringField(required=True, description="Timestamp of when the program was built.")
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    compiler_version = jsl.StringField(required=True, description="Compiler version used in compilation.")
    schema_version = jsl.StringField(required=True, description="Schema version used to produce this JSON.")
    target = jsl.StringField(required=True, enum=TARGETS,
                             description="The target device this program was compiled for.")

    compilation_time = jsl.StringField(required=True, description="The time, in seconds, it took to compile this program.")

    phv = jsl.DocumentField("Phv", as_ref=True, required=True, description="Phv resource information.")
    parser = jsl.DocumentField("Parser", as_ref=True, required=True, description="Parser resource information.")
    mau = jsl.DocumentField("Mau", as_ref=True, required=True, description="MAU resource information.")
    deparser = jsl.DocumentField("Deparser", as_ref=True, required=True, description="Deparser resource information.")
    clots = jsl.DocumentField("Clots", as_ref=True, required=True, description="CLOT-usage information.")

#!/usr/bin/env python3

"""
mau_schema.py: Generates a JSON Schema model for structured MAU allocation result information.
"""

import jsl
import json
import inspect


########################################################
#   Schema Version
########################################################

"""
Version Notes:

1.0.0:
- Initial release
"""

major_version = 1
medium_version = 0
minor_version = 0

def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(medium_version), str(minor_version))

########################################################

# ------------------------------
#  Table information
# ------------------------------

class ActionParameterInfo(jsl.Document):
    title = "ActionParameterInfo"
    description = "Information about the action parameters used in an action."

    action_name = jsl.StringField(required=True, description="The name of the action.")
    parameters = jsl.ArrayField(items=jsl.DictField(
        properties={
            "name": jsl.StringField(required=True, description="The name of the action parameter."),
            "bit_width": jsl.IntField(required=True, description="The bit width of this action parameter.")
        }, required=True, description="", additional_properties=False),
        required=True, description="Array of action parameters used in this action.")

class StageEntryFormat(jsl.Document):

    title = "StageEntryFormat"
    description = "Information about a stage entry pack format."

    entries = jsl.ArrayField(items=jsl.DictField(
        properties={
            "entry_number": jsl.IntField(required=True, description="An integer indicating the entry number."),
            "fields": jsl.ArrayField(items=jsl.DictField(
                properties={
                    "name": jsl.StringField(required=True, description="The name of the field."),
                    "bit_width": jsl.IntField(required=True, description="The bit width of this field (or field slice)."),
                    "start_bit": jsl.IntField(required=True, description="An integer indicating the start bit, from the least significant bit, for this field."),
                    "memory_start_bit": jsl.IntField(required=True, description="An integer indicating the start bit in the memory table word, from the least significant bit, for this field.")
                }, required=True, description="", additional_properties=False))
        }, required=True, description="", additional_properties=False),
        required=True, description="Information about each entry in the packing")


class StageActionParameterMap(jsl.Document):

    title = "StageActionParameterMap"
    description = "Information about a specific action parameter (or parameter slice) mapping to action bus usage."

    name = jsl.StringField(required=True, description="The action parameter name.")
    bit_width = jsl.IntField(required=True, description="The bit width of this parameter (or parameter slice).")
    start_bit = jsl.IntField(required=True, description="An integer indicating the start bit, from the least significant bit, for this parameter.")
    # Should we include the action parameter slot position for the parameter slice?
    location = jsl.StringField(required=True, enum=["action_data", "match_overhead"], description="The location of this action parameter (or parameter slice), either an action data table or immediate match overhead.")

    action_data_slot_size = jsl.IntField(required=True, enum=[8, 16, 32], description="The bit width of this action slot size this action parameter (or parameter slice) is mapped to.")
    action_slot_number = jsl.IntField(required=True, enum=list(range(0, 32)), description="The action slot number this action parameter (or parameter slice) is mapped to.")


class StageActionFormat(jsl.Document):

    title = "StageActionFormat"
    description = "Information about a specific action formatting in a stage."

    name = jsl.StringField(required=True, description="The action name.")
    action_format = jsl.DocumentField("StageEntryFormat", as_ref=True, required=True,
                                      description="The action-specific pack format in this stage.")
    parameter_map = jsl.ArrayField(jsl.DocumentField("StageActionParameterMap", as_ref=True, required=True),
                                   required=True, description="A list of the mappings for action parameter (or parameter slice) to action data bus locations.")


# Statistics, meter, selection, and stateful memory usages use this base class.
class StageMemoryDetails(jsl.Document):
    title = "StageMemoryDetails"
    description = "Base information about memory use for a specific table type in a single stage."

    memory_type = jsl.StringField(required=True, enum=["sram", "tcam", "buf", "map_ram"],
                                  description="The type of memory being used.")
    table_word_width = jsl.IntField(required=True, description="Bit width of the table's word.")
    memory_word_width = jsl.IntField(required=True, description="Bit width of a single physical memory word.")
    entries_per_table_word = jsl.IntField(required=True, description="Number of entries that are packed into a given table word.")
    table_type = jsl.StringField(required=True, enum=["action", "match" , "meter", "selection", "stateful", "statistics", "tind"],
                                  description="The table type the memory is used for.")
    num_memories = jsl.IntField(required=True, description="The total number of memory blocks in use.")


# Ternary indirection memory usages use this class.
class StageMemoryDetailsWithEntryWidth(StageMemoryDetails):
    title = "StageMemoryDetailsWithEntryWidth"
    description = "Information about memory use for a table in a single stage, including entry bit widths requested/allocated."

    entry_bit_width_requested = jsl.IntField(required=True, description="The expected entry bit width (including overhead), assuming ideal packing.")
    entry_bit_width_allocated = jsl.IntField(required=True, description="The entry bit width actually allocated.")


# Action data table memory usages use this class.
class StageMemoryDetailsWithEntryWidthAndIdeal(StageMemoryDetailsWithEntryWidth):
    title = "StageMemoryDetailsWithEntryWidthAndIdeal"
    description = "Information about memory use for a table in a single stage, including entry bit widths requested/allocated and ideal packing."

    ideal_entries_per_table_word = jsl.IntField(required=True, description="The ideal number of entries per ideal table word width, based only on the match key bit width.")
    ideal_table_word_bit_width = jsl.IntField(required=True, description="The ideal table word width that uses the most number of memory bits, assuming no packing constraints.")


# Match table memory usages use this class.
class StageMatchMemoryDetails(StageMemoryDetailsWithEntryWidthAndIdeal):
    title = "StageMatchMemoryDetails"
    description = "Information about match memory use for a table in a single stage."

    entries_requested = jsl.IntField(required=True, description="The number of entries requested to go in this stage.")
    entries_allocated = jsl.IntField(required=True, description="The number of entries actually allocated in this stage.")
    imm_bit_width_in_overhead_requested = jsl.IntField(required=True, description="The immediate match overhead bit width expected, assuming ideal packing.")
    imm_bit_width_in_overhead_allocated = jsl.IntField(required=True, description="The immediate match overhead bit width actually allocated.")


class MatchTables(jsl.Document):

    class StageDetails(jsl.Document):
        title = "StageDetails"
        description = "Information about packing and resource usage on a per-stage basis."

        stage_number = jsl.IntField(required=True, description="The MAU stage where this section of the table resides.")

        memories = jsl.ArrayField(items=jsl.OneOfField([
            jsl.DocumentField("StageMemoryDetails", as_ref=True),  # meter, selection, stateful, statistics
            jsl.DocumentField("StageMatchMemoryDetails", as_ref=True),   # match
            jsl.DocumentField("StageMemoryDetailsWithEntryWidthAndIdeal", as_ref=True),  # action data
            jsl.DocumentField("StageMemoryDetailsWithEntryWidth", as_ref=True)]),  # tind
        required=True, description="A list of how this table uses memory in this stage, including any attached tables it may have.")

        # Needed?
        overhead_fields = jsl.ArrayField(items=jsl.DictField(
            properties={
                "name": jsl.StringField(required=True,
                                        enum=["version", "version/valid", "next_table", "action_instr", "action_data_addr", "meter_addr", "stateful_addr", "selector_addr", "selector_length", "selector_shift", "statistics_addr", "immediate"],
                                        description="The name of the overhead field."),
                "bit_width": jsl.IntField(required=True, description="The number of bits allocated for the overhead field.")
            }, required=True, description="", additional_properties=False),
            required=True, description="A list of the overhead fields that are used in each match entry, if any.")

        match_format = jsl.DocumentField("StageEntryFormat", as_ref=True, required=True,
                                         description="The match table's match pack format in this stage.")

        action_formats = jsl.ArrayField(items=jsl.DocumentField("StageActionFormat", as_ref=True),
                                        required=True, description="The action table's formatting and action bus usage on a per-action basis.")

    title = "Tables"
    description = "Table information."

    name = jsl.StringField(required=True, description="The name of the table.")
    gress = jsl.StringField(required=True, enum=["ingress", "egress", "ghost"], description="The gress the table belongs to.")
    lookup_types = jsl.ArrayField(items=jsl.StringField(enum=["exact", "ternary", "lpm", "range"]), required=True,
                                  description="The lookup types used by this table.")

    entries_requested = jsl.IntField(required=True, description="The total number of table entries requested.")
    entries_allocated = jsl.IntField(required=True, description="The total number of table entries allocated.")

    # Needed?
    match_fields = jsl.ArrayField(items=jsl.DictField(
        properties={
            "name": jsl.StringField(required=True, description="The name of a field in the match key."),
            "start_bit": jsl.IntField(required=True,
                                      description="An integer indicating the start bit, from the least significant bit, to use for the field."),
            "bit_width": jsl.IntField(required=True, description="The number of bits used from the match key field."),
            "lookup_type": jsl.StringField(required=True, enum=["exact", "ternary", "lpm", "range"],
                                           description="The lookup type for this field.")
        }, required=True, description="", additional_properties=False),
        required=True, description="A list of the match fields that are used in each match entry, if any.")

    action_parameters = jsl.ArrayField(items=jsl.DocumentField("ActionParameterInfo", as_ref=True), required=True,
                             description="Action parameter information used per action.")

    stage_allocation = jsl.ArrayField(items=jsl.DocumentField("StageDetails", as_ref=True, description=""),
                                      required=True, description="An array of the per-stage allocation information for this match table.")


# ------------------------------
#  Top level JSON schema
# ------------------------------

class MauJSONSchema(jsl.Document):
    title = "MauJSONSchema"
    description = "MAU JSON schema for a P4 program's table placement results."
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    build_date = jsl.StringField(required=True, description="Timestamp of when the program was built.")
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    compiler_version = jsl.StringField(required=True, description="Compiler version used in compilation.")
    schema_version = jsl.StringField(required=True, description="Schema version used to produce this JSON.")

    tables = jsl.ArrayField(items=jsl.DocumentField("MatchTables", as_ref=True), required=True,
                            description="A list of match table allocation information.")

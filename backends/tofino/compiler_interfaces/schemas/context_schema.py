#!/usr/bin/env python3

"""
context_schema.py: Generates a JSON Schema model for resource allocation information
on the p4 compiler for Tofino.
"""

import jsl
import json
import inspect
import os.path
import sys
import collections

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

1.5.10:
- Initial version migrated.
1.5.11:
- Adds optional 'p4_hidden' argument to table objects to indicate that no PD APIs need to be generated.
  (The table is not associated with any P4-level object, was added by the compiler, and it should not be modified.)
1.5.12:
- Removes attributes: default_selector_value and default_selector_mask.
  These are no longer needed by the driver.
1.6.0:
- Structural changes for simplifying dynamic hashing information
  for use with bf-utils library.
1.6.1:
- Adds optional 'parsers' node for multiple parser program support. A 'parsers' node consists of multiple
  'ParserInstance' nodes. Each 'ParserInstance' node includes a list of 'PVS' nodes and a 'handle' field.
1.6.2:
- Updates the number of enum values for p4_hash_bit from 64 to 104.
1.6.3:
- Add 'nibble_offset' for range entries less than 4 bits wide
1.6.4:
- Adds 'target' attribute to specify the device target.
1.6.5:
- Adds 'default' attribute to 'ParserInstance' node to specify the default port id.
1.6.6:
- Adds 'name' attribute to 'ParserInstance' node to specify the name of parser instance for use with bfrt.
  Renamed 'default' attribute in 'ParserInstance' to 'default_parser_id'.
1.6.7:
- Added excluded_field_msb_bits attribute to ALPMMatchTableAttr object
1.7.0:
- Output values which can be >64 bits as hex strings. Applies to static entry
  value/mask, action format constant value
1.7.1:
- JBAY next table exec and long barnch values to use for default(miss) actions
- match_next_table_adr_default value to use for miss actions
1.7.2
- Added an optional color_mapram_addr_type to the ReferencedTable, specifically for meter tables
  as the driver requires this for the meter address type
1.7.3
- Added an optional global name in match_key_fields giving alias name for lookup
  in snapshot
1.7.4
- Add optional 'parser_handle' in Phase0MatchStageTable to associate phase0 tables with
  their respective parsers in a multi parser scenario. For a single parser there
  is only one parser and phase0 table and hence this field can be optional.
1.8.0
- Added required attribute 'sps_scramble_enable' to SelectionStageTable objects to indicate whether SPS scramble
  is on (True) or off (False).
1.8.1
- Added optional 'tcam_rows' to PHV records to cross-reference parser extractions where appropriate
1.8.2
- Add optional attribute 'next_tables' to ActionHandleFormat to list info
  of all next tables to be run for the given action
  To identify the correct next table location each entry will have a
  'next_table_name', 'next_table_logical_id' and 'next_table_stage_no'
1.8.3
- Add optional node 'flexible_headers' to publish compiler defined layouts for flexible headers
1.8.4
- Add optional attribute 'phv_offset' to learn quanta field nodes to indicate offset of phv
  slice in the p4 field.
1.8.5
- Add optional 'user_annotations' list of annotation value lists to 'tables', 'match_key_fields', 'p4_parameters', and
  'actions' attributes.  These user annotations are intended to be used as a mechanism for a user to pass through
  information from P4 source to be picked up by backend tools.  One main motivating example is value formatting.
  Consider adding @user_annotation("formatting", "IPv4") to a match key field ipv4.dstAddr.  This would indicate
  whenever the value for ipv4.dstAddr should be displayed in a CLI (for example), that it should be formatted like
  a typical IPv4 address.
  All annotation parameters are treated as strings.  A user_annotation must have at least one element.
1.9.0
- Add required attribute "allowed_as_hit_action" to action objects.  This attribute
  is used to indicate if an action can be used when a match table hits.
1.10.0
- Rename 'stage_dependency' node to 'mau_stage_characteristics' node.
  Add required attributes to per-gress, per-stage mau_stage_characteristics node:
  'clock_cyles' indicates the clock cycle latency of a given MAU
  'predication_cycle' indicates the clock cycle in which predication occurs
  'cycles_contribute_to_latency' indicates the number of clock cycles this stage contributes to the overall pipeline gress latency
1.11.0
- Add 'handle' attribute to indirect_resources dictionary under actions.
  Add new 'direct_resources' dictionary under actions.
1.11.1
- Add 'mau_stage_extension' section with register programming to fill additional stages in the
  pipeline past what the binary programs
1.11.2
- Update 'spare_bank_memory_unit' and 'stats_alu_index' to accept vector to cover the case from which a
  single synthetic two-port table can use more than one Spare bank and ALU.
1.11.3
- Add 'position' to 'match_key_fields' section. This has been output by bf-asm for some time.
- Add 'mask' to 'match_key_fields' section to identify disjointed bit ranges e.g. key = {hdr.h.f 0x81 : exact;}
1.11.4
- Added 'atcam_subset_width', 'shift_granularity', 'max_number_actions', 'lpm_key_mask' to
  ALPM table for an optimization. Added new key field to alpm pre-classifier table if the optimization
  is used.
1.11.5
- Update 'meter_alu_index' to accept vector to cover the case from which a single synthetic two-port
  table can use more than one ALU.
1.11.6
- Added 'tof1_egr_parse_depth_checks_disabled' to driver_options to indicate
  when the Tofino 1 egress parse depth checks are explicitly disabled. In this
  case, the driver should disable multi-threading in the parser.
1.12.0
- Add required 'global_name' to hash_bits and ghost_bit_info in HashFunction
  Table and to Ternary / Exact Entry Formats. This field represents the original
  field name during aliasing / name annotations / field slicing associated with
  field names
1.12.1
- Make contents of ghost_bit_info required params. While ghost_bit_info can itself
  be an optional parameter, if present its contents should always be required.
1.12.2
- Add 'always_run' optional field to StageTable node (Tofino2+)
- Add optional next_tables & next_table_names nodes to allow for multiple next tables for Gateway Node (Tofino2+)
1.12.3
- add 'physical_table_id' to stage table
- split local/global exec (more than 32 bits total needed)
- add additional memory types to MemoryResourceAllocation
- require memory_type in HashMatchMemoryResourceAllocation and increase flexibility of bit locations for hash parameters, and add number_subword_bits
1.12.4
- add 'valid' as field type in ExactEntryFormat (1-bit experimental valid bit)
"""

major_version = 1
medium_version = 12
minor_version = 4

def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(medium_version), str(minor_version))

########################################################


user_annotations_var = jsl.ArrayField(required=False,
                                      items=jsl.ArrayField(min_items=1, items=jsl.StringField(),
                                                           description="Annotation values as strings."),
                                      description="Array of user-provided annotation tuples.  The order of the tuples "
                                                  "should be based on the program order.")

#### ENTRY FORMATS ####

class ActionDataEntryFormat(jsl.Document):
    title = "ActionDataEntryFormat"
    description = "Context information for the entries on an ActionDataTable."

    entry_number = jsl.IntField(required=True, description="Entry number uniquely identifying this entry.")
    fields = jsl.ArrayField(items=jsl.DictField(
        properties={
            "field_name": jsl.StringField(required=True, description="Name of this field."),
            "field_width": jsl.IntField(required=True, description="Width of this field, in bits."),
            "lsb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the least significant bit of this field."),
            "lsb_mem_word_offset": jsl.IntField(required=True, description="Offset of the least significant bit of the field within this memory word."),
            "const_tuples": jsl.ArrayField(items=jsl.DictField(properties={
                "dest_start": jsl.IntField(required=True, description="Index of the least significant bit of this segment of the constant."),
                "value": jsl.IntField(required=True, description="Value of this segment of the constant."),
                "dest_width": jsl.IntField(required=True, description="Width of this segment of the constant.")
            }), description="A constant may be split over multiple entries. This list contains information about each segment of a constant."),
            "source": jsl.StringField(required=True, description="Type of source that originates this entry.", enum=[
                'zero',
                'constant',
                'spec'
            ]),
            "start_bit": jsl.IntField(required=True, description="A field could be split in multiple slices. Corresponds to the offset, from the least significant bit, where this slice starts."),
            "is_mod_field_conditionally_mask": jsl.BooleanField(description="Indicates whether this field is the mask parameter for a modify_field_conditionally primitive.  The API passes in a Boolean, which needs to be replicated into each bit position in this field."),
            "is_mod_field_conditionally_value": jsl.BooleanField(description="Indicates whether this field is the value parameter for a modify_field_conditionally primitive."),
            "mod_field_conditionally_mask_field_name": jsl.StringField(description="Specifies the name of the modify field conditionally condition action parameter.  If that mask field is False (0), the value to encode for this 'value' action parameter has to be 0, regardless of what the API passes in as the value."),
        }, description="Description of a field in an action data stage table."),
        required=True,
        description="List of fields in this table.")

class TernEntryFormat(jsl.Document):
    title = "TernEntryFormat"
    description = "Context information for the entries on a Ternary MatchStageTable."

    entry_number = jsl.IntField(required=True, description="Entry number uniquely identifying this entry.")
    fields = jsl.ArrayField(items=jsl.DictField(
        properties={
            "field_name": jsl.StringField(required=True, description="Name of this field."),
            "global_name": jsl.StringField(required=True, description="Global name of this field."),
            "field_width": jsl.IntField(required=True, description="Width of this field, in bits."),
            "lsb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the least significant bit of this field."),
            "msb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the most significant bit of this field."),
            "lsb_mem_word_offset": jsl.IntField(required=True, description="Offset, within a table word, corresponding to the start of this field, starting from the least significant bit."),
            "start_bit": jsl.IntField(required=True, description="A field could be split in multiple slices. Corresponds to the offset, from the least significant bit, where this slice starts."),
            "source": jsl.StringField(required=True, description="Type of source that originated this entry.", enum=[
                'version',
                'zero',
                'parity',
                'payload',
                'range',
                'spec'
            ]),
            "range": jsl.DictField(properties={
                "type": jsl.IntField(required=True, enum=[2, 4], description="Range match type, indicating 2-bit DirtCAM or 4-bit DirtCAM."),
                "is_duplicate": jsl.BooleanField(required=True,
                    description="Whether this range field is a duplicate generated by the compiler or not."),
                "nibble_offset": jsl.IntField(required=False,
                    description="Offset where a field or field slice starts within a range nibble. Absence indicates start is at 0")
            }, description="Range field information. This value is optional, and will only appear if the field uses ranges.")
        },
        description="Description of a field in a ternary stage table."),
    required=True,
    description="List of fields in this table.")

class TindEntryFormat(jsl.Document):
    title = "TindEntryFormat"
    description = "Context information for the entries on a Ternary Indirection MatchStageTable."

    entry_number = jsl.IntField(required=True, description="Entry number uniquely identifying this entry.")
    fields = jsl.ArrayField(items=jsl.DictField(
        properties={
            "field_name": jsl.StringField(required=True, description="Name of this field."),
            "field_width": jsl.IntField(required=True, description="Width of a field in bits."),
            "source": jsl.StringField(required=True, description="Type of source that originated this entry.", enum=[
                'zero',
                'instr',
                'adt_ptr',
                'meter_ptr',
                'stats_ptr',
                'stful_ptr',
                'sel_ptr',
                'next_table',
                'selection_length',
                'selection_length_shift',
                'immediate'
            ]),
            "lsb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the least significant bit of this field."),
            "msb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the most significant bit of this field."),
            "lsb_mem_word_offset": jsl.IntField(required=True, description="Offset, within a table word, corresponding to the start of this field, starting from the least significant bit."),
            "start_bit": jsl.IntField(required=True, description="A field could be split in multiple slices. Corresponds to the offset, from the least significant bit, where this slice starts."),
            "enable_pfe": jsl.BooleanField(required=True, description="Whether to enable or disable per flow enable functionality on this entry."),
            "immediate_name": jsl.StringField(description="Name of the [source] field, if it was an IMMEDIATE source.")
        }, description="Description of a field in a ternary indirection table."),
        required=True,
        description="List of fields in this table.")

class ExactEntryFormat(jsl.Document):
    title = "ExactEntryFormat"
    description = "Context information for an ExactMatchStageTable."

    entry_number = jsl.IntField(required=True, description="Entry number uniquely identifying this entry.")
    fields = jsl.ArrayField(items=jsl.DictField(properties={
        "field_name": jsl.StringField(required=True, description="Name of this field."),
        "global_name": jsl.StringField(required=True, description="Global name of this field."),
        "field_width": jsl.IntField(required=True, description="Width of a field, in bits."),
        "source": jsl.StringField(required=True, description="Type of source that originated this entry.", enum=[
            'zero',
            'version',
            'valid',
            'instr',
            'adt_ptr',
            'meter_ptr',
            'stats_ptr',
            'stful_ptr',
            'sel_ptr',
            'next_table',
            'selection_length',
            'selection_length_shift',
            'proxy_hash',
            'spec',
            'immediate'
        ]),
        "lsb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the least significant bit of this field."),
        "msb_mem_word_idx": jsl.IntField(required=True, description="Index of the wide word containing the most significant bit of this field."),
        "lsb_mem_word_offset": jsl.IntField(required=True, description="Offset, within a table word, corresponding to the start of this field, starting from the least significant bit."),
        "start_bit": jsl.IntField(required=True, description="A field could be split in multiple slices. Corresponds to the offset, from the least significant bit, where this slice starts."),
        "immediate_name": jsl.StringField(description="Name of the [source] field, if it was an IMMEDIATE source."),
        "enable_pfe": jsl.BooleanField(required=True, description="Whether to enable or disable per flow enable functionality on this entry."),
        "match_mode": jsl.StringField(enum=["exact", "s0q1", "s1q0", "unused"], description="Indicates the how the hardware is configured to match on this region.  Only applicable when the source is SPEC and for ATCAM tables.")
    }, required=True), required=True)

#### STAGE TABLES ####

class StageTable(jsl.Document):
    class ActionHandleFormat(jsl.Document):
        title = "ActionHandleFormat"
        description = "ActionHandleFormat context information."

        vliw_instruction = jsl.IntField(required=True, description="The VLIW instruction address.")
        vliw_instruction_full = jsl.IntField(required=True, description="Full VLIW address. This value is necessary in case this action is used as default..")
        next_table = jsl.IntField(required=True, description="The next table address for the driver to use when programming the match overhead.  This can be less than 8 bits if the hardware indirection table is used.")
        next_table_full = jsl.IntField(required=True, description="The full hardware address of the next table pointer to use when this action is called.  This is an 8-bit value consisting of {stage, logical table ID}.")
        next_table_exec = jsl.IntField(description="(Tofino2 only: 31 bit local/global exec map for next tables in the same and next stages")
        next_table_global_exec = jsl.IntField(description="(Experimental: 24 bit global exec map for next tables in following stages")
        next_table_local_exec = jsl.IntField(description="(Experimental: 15 bit local exec map for next tables in the same stages")
        next_table_long_brch = jsl.IntField(description="(Tofino2+ only: 8 bit long branch mask")
        next_tables = jsl.ArrayField(items=jsl.DictField(properties={
            "next_table_name": jsl.StringField(required=True, description="P4 next table name for the next table"),
            "next_table_logical_id": jsl.IntField(required=True, description="P4 next table logical id within the stage"),
            "next_table_stage_no": jsl.IntField(required=True, description="P4 next table stage no")
        }), required=False, description="List of all next tables for the given action")
        action_name = jsl.StringField(description="The name of the action.")
        action_handle = jsl.IntField(required=True, description="The handle of this particular action.")
        table_name = jsl.StringField(description="The name of the match table.")

        immediate_fields = jsl.ArrayField(items=jsl.AnyOfField([
            jsl.DictField(properties={
                "param_shift": jsl.IntField(required=True, description="The parameter start least significant bit stored in the immediate.  P4 parameters can be stored in match overhead and/or action data tables."),
                "param_name": jsl.StringField(required=True, description="The name of the parameter."),
                "param_type": jsl.StringField(required=True, enum=["parameter", "constant"], description="The type of this immediate field."),
                "dest_start": jsl.IntField(required=True, description="Multiple parameters/constants can be mapped to the match overhead immediate field.  This integer indicates the least significant bit in that match overhead field where this immediate field begins."),
                "dest_width": jsl.IntField(required=True, description="Width of the slice being written to the immediate field."),
                "is_mod_field_conditionally_mask": jsl.BooleanField(description="Indicates whether this field is the mask parameter for a modify_field_conditionally primitive.  The API passes in a Boolean, which needs to be replicated into each bit position in this field."),
                "is_mod_field_conditionally_value": jsl.BooleanField(description="Indicates whether this field is the value parameter for a modify_field_conditionally primitive."),
                "mod_field_conditionally_mask_field_name": jsl.StringField(description="Specifies the name of the modify field conditionally condition action parameter.  If that mask field is False (0), the value to encode for this 'value' action parameter has to be 0, regardless of what the API passes in as the value.")
            }, description="An action parameter stored in match overhead, for the case of a parameter (non-constant) overhead."),
            jsl.DictField(properties={
                "const_value": jsl.IntField(required=True, description="Indicates a constant value to be programmed."),
                "param_name": jsl.StringField(required=True, description="The name of the parameter."),
                "param_type": jsl.StringField(required=True, enum=["parameter", "constant"], description="The type of this immediate field."),
                "dest_start": jsl.IntField(required=True, description="Multiple parameters/constants can be mapped to the match overhead immediate field.  This integer indicates the least significant bit in that match overhead field where this immediate field begins."),
                "dest_width": jsl.IntField(required=True, description="Width of the slice being written to the immediate field.")
            }, description="An action parameter stored in match overhead, for the case of a constant (immediate) overhead.")]),
            required=True, description="An array of action parameters stored in the match overhead. The parameters can be either from the match spec, or constants.")

    class ATCAMMemoryResourceAllocation(jsl.Document):
        title = "ATCAMMemoryResourceAllocation"
        description = "ATCAMMemoryResourceAllocation context information."

        column_priority = jsl.IntField(required=True, description="Priority for this column. A lower value means higher priority.")
        memory_units_and_vpns = jsl.ArrayField(items=jsl.DictField(properties={
            "memory_units": jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of memory unit IDs.  For SRAMs, the memory unit ID is 12 * row + col.  For TCAMs, the memory unit ID is 12 * col + row.  (The formula difference is not a typo.)  Memory IDs are ordered such that the most significant memory appears at the lower list index."),
            "vpns": jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of VPNs assigned for the match entries packed in these memory units.")
        }), required=True, description="Array of bins of 1024 partitions that constitute a column.")

    class MemoryResourceAllocation(jsl.Document):
        title = "MemoryResourceAllocation"
        description = "MemoryResourceAllocation context information."

        memory_type = jsl.StringField(required=True, enum=['tcam', 'sram', 'map_ram', 'gateway', 'ingress_buffer', 'stm', 'scm', 'scm-tind', 'lamb'], description="Type of memory allocated.")
        spare_bank_memory_unit = jsl.AnyOfField([jsl.IntField(), jsl.ArrayField(items=jsl.IntField())],
            description="For synthetic two-port tables (stateful, meter, stats, selectors), indicates which memory unit(s) ID(s) is the spare banks initially.  This is used by the driver for initialization.")

        memory_units_and_vpns = jsl.ArrayField(items=jsl.DictField(properties={
            "memory_units": jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of memory unit IDs.  For SRAMs, the memory unit ID is 12 * row + col.  For TCAMs, the memory unit ID is 12 * col + row.  (The formula difference is not a typo.)  Memory IDs are ordered such that the most significant memory appears at the lower list index."),
            "vpns": jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of VPNs assigned for the match entries packed in these memory units.")
        }), required=True, description="Memory resource allocation information.")

    class HashMatchMemoryResourceAllocation(jsl.Document):
        title = "HashMatchMemoryResourceAllocation"
        description = "HashMatchMemoryResourceAllocation context information."

        memory_type = jsl.StringField(required=True, enum=['sram', 'stm', 'lamb'], description="Type of memory allocated.")
        memory_units_and_vpns = jsl.ArrayField(items=jsl.DictField(properties={
            "memory_units": jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of SRAM memory unit IDs.  The memory unit ID is 12 * row + col.  Memory IDs are ordered such that the most significant memory appears at the lower list index.  Memory IDs are ordered such that the most significant memory appears at the lower list index."),
            "vpns": jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of VPNs assigned for the match entries packed in these memory units.")
        }), required=True, description="Memory resource allocation information in this ExactMatchStageTable.")

        hash_function_id = jsl.IntField(required=True, description="Hash function ID that is used to compute the RAM enable select bits and the RAM word entry bits.")
        hash_select_bit_lo = jsl.IntField(required=True, description="Position of the least significant bit boundary in the hash value to use to activate a RAM unit.")
        hash_select_bit_hi = jsl.IntField(required=True, description="Position of the most significant bit boundary in the hash value to use to activate a RAM unit.")
        hash_entry_bit_lo = jsl.IntField(required=True, description="Position of the least significant bit boundary in the hash value to use to determine a RAM unit row.")
        hash_entry_bit_hi = jsl.IntField(required=True, description="Position of the most significant bit boundary in the hash value to use to determine a RAM unit row.")
        number_select_bits = jsl.IntField(required=True, description="Number of hash bits used for selecting the RAM. Values allowed: 0-12 (inclusive).")
        number_entry_bits = jsl.IntField(required=True, enum=list(range(6, 11)), description="Number of hash bits used for selecting the word in a RAM.")
        number_subword_bits = jsl.IntField(required=True, description="Number of hash bits used for subword selection")

    class StashAllocation(jsl.Document):
        title = "StashAllocation"
        description = "StashAllocation context information."

        pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "table_word_width": jsl.IntField(required=True, description="Bit width of the stash word.  Note that this can be less than the corresponding exact match table's word width."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical stash word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word.  For stashes, this can only be 1."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit."),
            "entries": jsl.ArrayField(items=jsl.DocumentField("ExactEntryFormat"), required=True, description="Entries packed in this stash.")
        }, required=True, description="Structure containing information about how the stash for this stage table is layed out."))

        num_stash_entries = jsl.IntField(required=True, description="The number of wide-word stash entries available.")
        stash_entries = jsl.ArrayField(items=jsl.ArrayField(jsl.DictField(properties={
                "stash_entry_id": jsl.IntField(required=True, description="The physical IDs of the stash entries in use for one potentially wide word entry.  IDs are arranged with most significant part of the stash entry in the lowest index."),
                "stash_match_data_select": jsl.IntField(required=True, enum=[0, 1], description="Indicates which match data search bus on the physical row is input to this subsection of a stash entry."),
                "stash_hashbank_select": jsl.IntField(required=True, enum=[0, 1], description="Indicates which hash input on the physical row is input to these stash entries."),
                "hash_function_id": jsl.IntField(required=True, enum=[0, 1], description="The hash function ID these stash entries correspond to.  There can be at most two hash functions for an exact match table.  The second hash function is for additional ways.")
            }, required=True, description="Properties for this unit of stash entry width.  There is no guarantee that allocation will result in the same input and output bus IDs will be used on each physical row.")
            ), required=True, description="Structure containing information about how the stash for this stage table is layed out.")

    class ConditionMemoryResourceAllocation(jsl.Document):
        title = "ConditionMemoryResourceAllocation"
        description = "ConditionMemoryResourceAllocation context information."
        memory_unit = jsl.IntField(required=True, enum=list(range(0, 16)), description="An int representing the gateway unit number.")
        memory_type = jsl.StringField(required=True, enum=["gateway"], description="Type of memory allocated.")
        payload_buses = jsl.ArrayField(items=jsl.DictField(properties={
            "bus_id": jsl.IntField(required=True, description="The payload result bus ID."),
            "bus_type": jsl.StringField(required=True, enum=["exact", "ternary"], description="The type of payload result bus."),
            "data_value": jsl.IntField(required=True, description="The 64-bit value provided as the payload result when the gateway inhibits."),
            "match_address_value": jsl.IntField(required=True, description="The 19-bit value provided as the match address value when the gateway inhibits."),
        }), required=True, description="Memory resource allocation information in this ExactMatchStageTable.")


    title = "StageTable"
    description = "StageTable common context information."

    stage_table_type = jsl.StringField(required=True, enum=[
        'hash_way',
        'ternary_match',
        'ternary_indirection',
        'action_data',
        'direct_match',
        'hash_match',
        'selection',
        'idletime',
        'phase_0_match',
        'statistics',
        'hash_action',
        'meter',
        'algorithmic_tcam_match',
        'proxy_hash_match',
        'match_with_no_key',
        'gateway'
    ], description="Type of this stage table.")

    stage_number = jsl.IntField(required=True, description="Number identifying the stage containing this stage table.")
    logical_table_id = jsl.IntField(required=True, description="Unique identifier for this logical table within this stage.")
    physical_table_id = jsl.IntField(required=False, description="Physical table number for this match table (Experimental)")
    size = jsl.IntField(required=True, description="Number of entries in this stage table.")

    pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "table_word_width": jsl.IntField(required=True, description="Bit width of the table's word."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical memory word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit.")
        }, required=True, description="Structure containing information about how the memory for this stage table is layed out."))

    memory_resource_allocation = jsl.OneOfField([
        jsl.NullField(),
        jsl.DocumentField("MemoryResourceAllocation", as_ref=True)
    ], required=True, description="Representation of the Memory resource allocation for this stage table.")
    always_run = jsl.BooleanField(required=False,
                    description="In Tofino2+ archs, a table can be marked as always_run when it is always enabled for a stage")

class HashWayStageTable(StageTable):
    title = "HashWayStageTable"
    description = "HashWayStageTable context information."

    way_number = jsl.IntField(required=True, description="The number identifying this hash way.")
    logical_table_id = jsl.IntField(description="Unique identifier for this stage table within the stage. This is not required for Phase0.")
    stage_table_type = jsl.StringField(required=True, enum=['hash_way'], description="Type of stage table.")

    pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "table_word_width": jsl.IntField(required=True, description="Bit width of the table's word."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical memory word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit."),
            "entries": jsl.ArrayField(items=jsl.DocumentField("ExactEntryFormat"), required=True, description="Entries packed in this stage table's memory.")
        }, required=True, description="Structure containing information about how the memory for this stage table is layed out."))

    memory_resource_allocation = jsl.OneOfField([
        jsl.DocumentField("HashMatchMemoryResourceAllocation", description="Representation of the memory resource allocation for this stage table. This structure is particular to a HashWayStageTable."),
        jsl.NullField()
    ], required=True)

class HashFunction(jsl.Document):
    title = "HashFunction"
    description = "Description of a hash function used in a stage table."

    hash_function_number = jsl.IntField(required=True, description="The number identifying this hash function in this stage table. This number can currently only be 0 or 1, and it corresponds to the hash functions used by different way groups. The lower ways will use hash function 0, and the higher ways will use hash function 1.")
    hash_bits = jsl.ArrayField(items=jsl.DictField(properties={
        "hash_bit": jsl.IntField(required=True, description="Identifier for this particular hash bit."),
        "seed": jsl.IntField(required=True, enum=[0, 1], description="Seed value for this particular hash bit."),
        "bits_to_xor": jsl.ArrayField(items=jsl.DictField(properties={
            "field_name": jsl.StringField(required=True, description="Identifier for this particular hash bit."),
            "global_name": jsl.StringField(required=True, description="Global Identifier for this particular hash bit."),
            "field_bit": jsl.IntField(required=True, description="Bit within the byte of the field corresponding to this hash."),
            "hash_match_group_bit": jsl.IntField(required=True, description="Bit within the hash match group corresponding to this bit."),
            "hash_match_group": jsl.IntField(required=True, description="Number identifying the hash match group in which this bit resides.")
        }, description="Description of bit from the match spec to include in the hash computation for a particular hash output bit."))
    }), required=True, description="Array of match spec bit properties to include in the calculation for each particular hash result output bit.")
    ghost_bit_info = jsl.ArrayField(items=jsl.DictField(properties={
        "field_name": jsl.StringField(required=True, description="Name of the field."),
        "global_name": jsl.StringField(required=True, description="Global name of the field."),
        "bit_in_match_spec": jsl.IntField(required=True, description="Bit within the field where this ghost bit lives.")
    }, required=False, description="Information on ghost bits for this stage."))
    ghost_bit_to_hash_bit = jsl.ArrayField(items=jsl.ArrayField(items=jsl.IntField(required=False, description="Hash bit within this hash function for which this ghost bit participates"), required=False, description="ghost bits to hash bits"), required=False, description="Array of ghost bits to hash bits, one per ghost bit")

class ExactMatchStageTable(StageTable):
    title = "ExactMatchStageTable"
    description = "ExactMatchStageTable context information."

    hash_functions = jsl.ArrayField(items=jsl.DocumentField("HashFunction", as_ref=True), required=True, description="Array of hash functions for this match table.")

    result_physical_buses = jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of integers corresponding to the physical bus numbers used by this stage table. This information is required for snapshot.")
    has_attached_gateway = jsl.BooleanField(required=True, description="A Boolean indicating if this stage table has an attached gateway.")

    ways = jsl.ArrayField(items=jsl.DocumentField("HashWayStageTable", as_ref=True), required=True, description="Context information for each particular way in this exact match stage table.")

    action_format = jsl.ArrayField(items=jsl.DocumentField("ActionHandleFormat", as_ref=True), required=True, description="List of action handles for this exact match stage table.")

    memory_resource_allocation = jsl.OneOfField([
        jsl.DocumentField("HashMatchMemoryResourceAllocation", as_ref=True, description="Representation of the memory resource allocation for this stage table. This structure is particular to ExactMatchStageTables, except for ATCAM."),
        jsl.NullField()], required=True)

    stash_allocation = jsl.OneOfField([
        jsl.DocumentField("StashAllocation", as_ref=True, description="Representation of the stash resource allocation for this stage table. This structure is particular to ExactMatchStageTables, except for ATCAM."),
        jsl.NullField()], required=True)

    default_next_table = jsl.OneOfField([jsl.IntField(), jsl.NullField()], required=True, description="Full address of the default next table, if any.")
    idletime_stage_table = jsl.DocumentField("IdletimeStageTable", as_ref=True, description="Idletime stage table associated to this stage table.")

class HashMatchStageTable(ExactMatchStageTable):
    title = "HashMatchStageTable"
    description = "HashMatchStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['hash_match'], description="Type of exact match stage table.")

class HashActionStageTable(StageTable):
    title = "HashActionStageTable"
    description = "HashActionStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['hash_action'], description="Type of exact match stage table.")

    hash_functions = jsl.ArrayField(items=jsl.DocumentField("HashFunction", as_ref=True), required=True, description="Array of hash functions for this match table.")

    result_physical_buses = jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of integers corresponding to the physical bus numbers used by this stage table. This information is required for snapshot.")
    has_attached_gateway = jsl.BooleanField(required=True, description="A Boolean indicating if this stage table has an attached gateway.")

    default_next_table = jsl.OneOfField([jsl.IntField(), jsl.NullField()], required=True, description="Full address of the default next table, if any.")
    action_format = jsl.ArrayField(items=jsl.DocumentField("ActionHandleFormat", as_ref=True), required=True, description="List of action handles for this exact match stage table.")

    memory_resource_allocation = jsl.NullField()
    idletime_stage_table = jsl.DocumentField("IdletimeStageTable", as_ref=True, description="Idletime stage table associated to this stage table.")

class DirectMatchStageTable(ExactMatchStageTable):
    title = "DirectMatchStageTable"
    description = "DirectMatchStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['direct_match'], description="Type of exact match stage table.")

class ProxyHashMatchStageTable(ExactMatchStageTable):
    title = "ProxyHashMatchStageTable"
    description = "ProxyHashMatchStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['proxy_hash_match'], description="Type of exact match stage table.")

    proxy_hash_bit_width = jsl.IntField(required=True, description="An integer indicating the bit width of proxy value stored in place of the normal exact match key.")
    proxy_hash_algorithm = jsl.StringField(description="The name of the hash algorithm used to compute the proxy value.")

    hash_functions = jsl.ArrayField(items=jsl.DocumentField("HashFunction", as_ref=True), required=True, description="Array of hash functions for this match table.")
    proxy_hash_function = jsl.DocumentField("HashFunction", as_ref=True, required=True, description="Match spec bit properties to include in the calculation for the proxy hash function.")

class MatchWithNoKeyStageTable(StageTable):
    title = "MatchWithNoKeyStageTable"
    description = "MatchWithNoKeyStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['match_with_no_key'], description="Type of exact match stage table.")

    result_physical_buses = jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of integers corresponding to the physical bus numbers used by this stage table. This information is required for snapshot.")
    has_attached_gateway = jsl.BooleanField(required=True, description="A Boolean indicating if this stage table has an attached gateway.")

    default_next_table = jsl.OneOfField([jsl.IntField(), jsl.NullField()], required=True, description="Full address of the default next table, if any.")
    action_format = jsl.ArrayField(items=jsl.DocumentField("ActionHandleFormat", as_ref=True), required=True, description="List of action handles for this exact match stage table.")

class ATCAMMatchStageTable(StageTable):
    title = "ATCAMMatchStageTable"
    description = "ATCAMMatchStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['algorithmic_tcam_match'], description="Type of exact match stage table.")

    default_next_table = jsl.OneOfField([jsl.IntField(), jsl.NullField()], required=True, description="Full address of the default next table, if any.")
    memory_resource_allocation = jsl.ArrayField(jsl.DocumentField("ATCAMMemoryResourceAllocation", as_ref=True), required=True, description="A representation of the memory resources used by this ATCAM stage table.")

    hash_functions = jsl.ArrayField(items=jsl.DocumentField("HashFunction", as_ref=True), required=True, description="Array of hash functions for this match table.")

    result_physical_buses = jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of integers corresponding to the physical bus numbers used by this stage table. This information is required for snapshot.")
    has_attached_gateway = jsl.BooleanField(required=True, description="A Boolean indicating if this stage table has an attached gateway.")

    action_format = jsl.ArrayField(items=jsl.DocumentField("ActionHandleFormat", as_ref=True), required=True, description="List of action handles for this exact match stage table.")

    idletime_stage_table = jsl.DocumentField("IdletimeStageTable", as_ref=True, description="Idletime stage table associated to this stage table.")

class Phase0MatchStageTable(StageTable):
    title = "Phase0MatchStageTable"
    description = "Phase0MatchStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['phase_0_match'], description="Type of exact match stage table.")
    parser_handle = jsl.IntField(required=False, description="Handle of the parser associated with this Phase0 Table. This value is optional for a single parser scenario.")

    logical_table_id = jsl.IntField(description="Unique identifier for this stage table within the stage. This is not required for Phase0.")

    pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "table_word_width": jsl.IntField(required=True, description="Bit width of the table's word."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical memory word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit."),
            # FIXME: Add Phase0 entry format.
            "entries": jsl.ArrayField()
        }, required=True, description="Structure containing information about how the memory for this stage table is layed out."))

class TernMatchStageTable(StageTable):
    title = "TernMatchStageTable"
    description = "TernMatchStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['ternary_match'], description="Type of match stage table.")

    pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "table_word_width": jsl.IntField(required=True, description="Bit width of the table's word."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical memory word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit."),
            "entries": jsl.ArrayField(items=jsl.DocumentField("TernEntryFormat"), required=True, description="Entries packed in this stage table's memory.")
        }, required=True, description="Structure containing information about how the memory for this stage table is layed out."))

    ternary_indirection_stage_table = jsl.DocumentField("TindStageTable", as_ref=True, description="Ternary indirection stage table associated to this stage table.")
    idletime_stage_table = jsl.DocumentField("IdletimeStageTable", as_ref=True, description="Idletime stage table associated to this stage table.")
    default_next_table = jsl.OneOfField([jsl.IntField(), jsl.NullField()], required=True, description="Full address of the default next table, if any.")

    result_physical_buses = jsl.ArrayField(items=jsl.IntField(), required=True, description="An array of integers corresponding to the physical bus numbers used by this stage table. This information is required for snapshot.")
    has_attached_gateway = jsl.BooleanField(required=True, description="A Boolean indicating if this stage table has an attached gateway.")

class StatisticsStageTable(StageTable):
    title = "StatisticsStageTable"
    description = "StatisticsStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['statistics'], description="Type of stage table.")
    stats_alu_index = jsl.AnyOfField([
            jsl.IntField(enum=list(range(0, 4))),
            jsl.ArrayField(items=jsl.IntField(enum=list(range(0, 4))))],
            required=True, description="An integer or an integer vector that indicates the statistics ALU(s) used.  0 is on row 0, 1 on row 2, 2 on row 4 and 3 on row 6.")

class MeterStageTable(StageTable):
    title = "MeterStageTable"
    description = "MeterStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['meter'], description="Type of stage table.")

    default_lower_huffman_bits_included = jsl.IntField(required=True, enum=[7], description="An integer that indicates how many of the hardware meter address bits are being provided by default configuration registers programmed by the compiler.  The meter address field in match overhead never includes the 7 least significant bits of the hardware meter address.")
    meter_alu_index = jsl.AnyOfField([
            jsl.IntField(enum=list(range(0, 4))),
            jsl.ArrayField(items=jsl.IntField(enum=list(range(0, 4))))],
            required=True, description="An integer or an integer vector that indicates the meter ALU(s) used.  0 is on row 1, 1 on row 3, 2 on row 5 and 3 on row 7.")
    color_memory_resource_allocation = jsl.OneOfField([
        jsl.NullField(),
        jsl.DocumentField("MemoryResourceAllocation", as_ref=True)], required=True, description="Representation of the color memory resource allocation for this meter.")

class SelectionStageTable(StageTable):
    title = "SelectionStageTable"
    description = "SelectionStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['selection'], description="Type of stage table.")
    meter_alu_index = jsl.IntField(required=True, enum=list(range(0, 4)), description="An integer that indicates the meter ALU used.  0 is on row 1, 1 on row 3, 2 on row 5 and 3 on row 7.")
    sps_scramble_enable = jsl.BooleanField(required=True, description="Boolean indicating if the selector ALU SPS scramble is enabled.")

class StatefulStageTable(StageTable):
    title = "StatefulStageTable"
    description = "StatefulStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['stateful'], description="Type of stage table.")
    meter_alu_index = jsl.IntField(required=True, enum=list(range(0, 4)), description="An integer that indicates the meter ALU used.  0 is on row 1, 1 on row 3, 2 on row 5 and 3 on row 7.")

class GatewayStageTable(StageTable):
    title = "GatewayStageTable"
    description = "GatewayStageTable context information."
    memory_resource_allocation = jsl.DocumentField("ConditionMemoryResourceAllocation", as_ref=True)
    next_table_names = jsl.DictField(properties={
        "true": jsl.AnyOfField([jsl.ArrayField(items=jsl.StringField(required=True,
                            description="The name of the next table(s) to run if the condition evalutes to true.")),
                               jsl.StringField(required=True,
                            description="The name of the next table(s) to run if the condition evalutes to true.")]),
        "false": jsl.AnyOfField([jsl.ArrayField(items=jsl.StringField(required=True,
                            description="The name of the next table(s) to run if the condition evalutes to false.")),
                               jsl.StringField(required=True,
                            description="The name of the next table(s) to run if the condition evalutes to false.")]),
    }, required=False, description="The next table branch locations (names) for each expression result.")
    next_tables = jsl.DictField(properties={
        "true": jsl.AnyOfField([jsl.ArrayField(items=jsl.IntField(required=True,
                        description="The full hardware address of the next tables to run if the condition evalutes to true. This is anv 8-bit value consisting of {stage, logical table ID}.")),
                                jsl.StringField(items=jsl.IntField(required=True,
                    description="The full hardware address of the next table to run if the condition evalutes to true. This is anv 8-bit value consisting of {stage, logical table ID}."))]),
        "false": jsl.AnyOfField([jsl.ArrayField(items=jsl.IntField(required=True,
                        description="The full hardware address of the next tables to run if the condition evalutes to false. This is anv 8-bit value consisting of {stage, logical table ID}.")),
                                jsl.StringField(items=jsl.IntField(required=True,
                    description="The full hardware address of the next table to run if the condition evalutes to false. This is anv 8-bit value consisting of {stage, logical table ID}."))]),
    }, required=True, description="The next table branch locations (full next table address) for each expression result.")
    stage_table_type = jsl.StringField(required=True, enum=['gateway'], description="Type of stage table.")

class IdletimeStageTable(StageTable):
    title = "IdletimeStageTable"
    description = "IdletimeStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['idletime'], description="Type of stage table.")

    precision = jsl.IntField(required=True, description="An integer representing how many bits are stored per idletime entry.")
    disable_notification = jsl.BooleanField(required=True, description="A Boolean indicating that no CPU notification will be sent when the flow state changes.")
    two_way_notification = jsl.BooleanField(required=True, description="A Boolean indicating if the CPU will be notified when a flow goes from inactive to active.  (For flows that go from active to inactive, the CPU is notified by default.)")
    enable_pfe = jsl.BooleanField(required=True, description="A Boolean indicating that entry aging can be done on a per-flow basis.  Note that this reserves one of the states in the aging state machine to indicate the entry does not timeout.")

class TindStageTable(StageTable):
    title = "TindStageTable"
    description = "TindStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['ternary_indirection'], description="Type of stage table.")

    logical_table_id = jsl.IntField(description="Unique identifier for this stage table within the stage. This is not required for TernaryIndirection.")

    pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "table_word_width": jsl.IntField(required=True, description="Bit width of the table's word."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical memory word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit."),
            "entries": jsl.ArrayField(items=jsl.DocumentField("TindEntryFormat"), required=True, description="Entries packed in this stage table's memory.")
        }, required=True, description="Structure containing information about how the memory for this stage table is layed out."))

    action_format = jsl.ArrayField(items=jsl.DocumentField("ActionHandleFormat", required=True, as_ref=True), required=True, description="List of action handles in this stage table.")

class ActionDataStageTable(StageTable):
    title = "ActionDataStageTable"
    description = "ActionDataStageTable context information."
    stage_table_type = jsl.StringField(required=True, enum=['action_data'], description="Type of stage table.")

    pack_format = jsl.ArrayField(items=jsl.DictField(properties={
            "action_handle": jsl.IntField(required=True, description="Action handle to which this pack format refers. On an ActionDataStageTable's pack format, there is one pack format per action."),
            "table_word_width": jsl.IntField(required=True, description="Bit width of the table's word."),
            "memory_word_width": jsl.IntField(required=True, description="Bit width of a single physical memory word."),
            "entries_per_table_word": jsl.IntField(required=True, description="Number of entries that are packed into a given table word."),
            "number_memory_units_per_table_word": jsl.IntField(required=True, description="Number of units combined together to form a single table word. Notice that for table words that occupy multiple memory units, the most significant bits of the table word are found in the lowest indexed memory unit."),
            "entries": jsl.ArrayField(items=jsl.DocumentField("ActionDataEntryFormat"), required=True, description="Entries packed in this stage table's memory.")
        }, required=True, description="Structure containing information about how the memory for this stage table is layed out."))

#### MATCH TABLE ATTRIBUTES ####

class MatchWithNoKeyTableAttr(jsl.Document):
    title = "MatchWithNoKeyTableAttributes"
    description = "Attributes of an MatchWithNoKeyTable context information."
    match_type = jsl.StringField(required=True, enum=['match_with_no_key'], description="Type of match table.")

    stage_tables = jsl.ArrayField(jsl.DocumentField("MatchWithNoKeyStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this match table. Keyless match tables should have only one of these.")

class HashActionTableAttr(jsl.Document):
    title = "HashActionTableAttributes"
    description = "Attributes of an HashActionTable context information."
    match_type = jsl.StringField(required=True, enum=['hash_action'], description="Type of match table.")

    stage_tables = jsl.ArrayField(jsl.DocumentField("HashActionStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this match table. Keyless match tables should have only one of these.")

class Phase0MatchTableAttr(jsl.Document):
    title = "Phase0MatchTableAttributes"
    description = "Attributes of an Phase0MatchTable context information."
    match_type = jsl.StringField(required=True, enum=['phase_0_match'], description="Type of match table.")

    stage_tables = jsl.ArrayField(jsl.DocumentField("Phase0MatchStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this match table. Phase 0 match tables should have only one of these.")

class ExactMatchTableAttr(jsl.Document):
    title = "ExactMatchTableAttributes"
    description = "Attributes of an ExactMatchTable context information."
    match_type = jsl.StringField(required=True, enum=['exact'], description="Type of match table.")

    prefix_length = jsl.IntField(description="Bit width of the prefix field. Only applicable if this table is also a CLPM unit.")
    prefix_upper = jsl.IntField(description="Maximum prefix length to be used. Only applicable if this table is also a CLPM Unit.")
    prefix_lower = jsl.IntField(description="Minimum prefix length to be used. Only applicable if this table is also a CLPM Unit.")

    uses_dynamic_key_masks = jsl.BooleanField(required=True, description="Whether this table can use dynamic key masks or not.")

    stage_tables = jsl.ArrayField(items=jsl.AnyOfField([
        jsl.DocumentField("HashMatchStageTable", as_ref=True),
        jsl.DocumentField("HashActionStageTable", as_ref=True),
        jsl.DocumentField("ProxyHashMatchStageTable", as_ref=True),
        jsl.DocumentField("DirectMatchStageTable", as_ref=True),
    ]), required=True, description="An array of stage-specific resource objects for this match table.")

class TernMatchTableAttr(jsl.Document):
    title = "TernMatchTableAttributes"
    description = "Attributes of a TernMatchTable."
    match_type = jsl.StringField(required=True, enum=['ternary'], description="Type of match table.")

    prefix_length = jsl.IntField(description="Bit width of the prefix field. Only applicable if this table is also a CLPM unit.")
    prefix_upper = jsl.IntField(description="Maximum prefix length to be used. Only applicable if this table is also a CLPM Unit.")
    prefix_lower = jsl.IntField(description="Minimum prefix length to be used. Only applicable if this table is also a CLPM Unit.")

    stage_tables = jsl.ArrayField(items=jsl.DocumentField("TernMatchStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this match table.")

class ALPMMatchTableAttr(jsl.Document):
    title = "ALPMMatchTableAttributes"
    description = "Attributes of a ALPMMatchTable."
    match_type = jsl.StringField(required=True, enum=['algorithmic_lpm'], description="Type of match table.")

    pre_classifier = jsl.DocumentField("MatchTable", required=True, description="Match table that serves as the ternary pre-classifier table for this ALPM table.")
    atcam_table = jsl.DocumentField("MatchTable", required=True, description="ATCAM match table within this ALPM table.")

    bins_per_partition = jsl.IntField(required=True, description="Number of entries per partition in the ALPM table.")
    max_subtrees_per_partition = jsl.IntField(required=True, description="Maximum number of subtrees in a given ALPM partition.")
    partition_field_name = jsl.StringField(required=True, description="The name of the partition index field. Usually, this is defaulted by the compiler to some known value.")
    lpm_field_name = jsl.StringField(required=True, description="Name of the field that is being matched against.")
    set_partition_action_handle = jsl.AnyOfField([jsl.IntField(), jsl.ArrayField(items=jsl.IntField())], required=True, description="set partition action handle(s) for this ALPM table.")
    excluded_field_msb_bits = jsl.ArrayField(required=False, items=jsl.DictField(properties={
            "field_name": jsl.StringField(required=True, description="The name of match key field that has been optimized for generating ATCAM partitions."),
            "msb_bits_excluded": jsl.IntField(required=True, description="The number of most significant bits of this field that will be excluded from the ATCAM packing.")
        }, additional_properties=False), description="A list of the excluded most significant bits from particular fields.  Note that only one field is allowed to exclude a subset of the field.")

    # Added for ALPM optimization
    atcam_subset_width = jsl.IntField(required=False, description="The width of the slice of lpm match key used in the atcam stage.")
    shift_granularity = jsl.IntField(required=False, description="The granularity of right shifts in the set partition action.")

    # An ALPM table should not have stage tables: either it does not show up,
    # or it is the empty list.
    stage_tables = jsl.ArrayField(max_items=0)

class ATCAMMatchTableAttr(jsl.Document):
    title = "ATCAMMatchTableAttributes"
    description = "Attributes of a ATCAMMatchTable."
    match_type = jsl.StringField(required=True, enum=['algorithmic_tcam'], description="Type of match table.")

    units = jsl.ArrayField(required=True, items=jsl.DocumentField("MatchTable", as_ref=True), description="List of ATCAM algorithmic units related to this ATCAM table.")
    number_partitions = jsl.IntField(required=True, description="Number of partitions in this ATCAM table.")
    partition_field_name = jsl.StringField(required=True, description="Name of the field used to partition entries.")

    # An ATCAM table should not have stage tables: either it does not show up,
    # or it is the empty list.
    stage_tables = jsl.ArrayField(max_items=0)

class ATCAMMatchUnitAttr(jsl.Document):
    title = "ATCAMMatchUnitAttributes"
    description = "Attributes of a ATCAMMatchUnit."
    match_type = jsl.StringField(required=True, enum=['algorithmic_tcam_unit'], description="Type of match table.")

    stage_tables = jsl.ArrayField(jsl.DocumentField("ATCAMMatchStageTable", as_ref=True))

class CLPMMatchTableAttr(jsl.Document):
    title = "CLPMMatchTableAttributes"
    description = "Attributes of a CLPMMatchTable."
    match_type = jsl.StringField(required=True, enum=['chained_lpm'], description="Type of match table.")

    prefix_length = jsl.IntField(required=True, description="Bit width of the prefix field.")
    units = jsl.ArrayField(items=jsl.DocumentField("MatchTable", as_ref=True), required=True, description="List of match tables that are prefixes to this CLPM match.")

    # No stage tables here, actually.
    stage_tables = jsl.ArrayField(max_items=0)

#### TABLES ####

class Table(jsl.Document):
    title = "Table"
    description = "Common table context information."

    # Properties of a P4-level table.
    name = jsl.StringField(required=True, description="Name of this table.")
    handle = jsl.IntField(required=True, description="Unique identifier for this table.")
    size = jsl.IntField(required=True, description="Minimum size of this table, as specified in P4.")
    direction = jsl.StringField(required=True, description="Whether this table is used for ingress, egress, or ghost.", enum=['ingress', 'egress', 'ghost'])

    p4_hidden = jsl.BooleanField(required=False,
                                 description="A Boolean indicating if this table was created by the compiler, "
                                             "has no P4-level object it is associated with, and should not have "
                                             "PD APIs generated for it.")
    user_annotations = user_annotations_var

class MatchTable(Table):
    class ReferencedTable(jsl.Document):
        title = "ReferencedTable"
        description = "Information about P4 tables referenced from another table."

        name = jsl.StringField(required=True, description="Name of the referenced table.")
        handle = jsl.IntField(required=True, description="Handle of the referenced table.")
        how_referenced = jsl.StringField(required=True, enum=['direct', 'indirect'], description="Whether the table is directly or indirectly referenced.")
        color_mapram_addr_type = jsl.StringField(required=False, enum=['idle', 'stats'], description="The address for the color mapram for default meter addresses")

    class StatefulAluExpressionDetails(jsl.Document):
        title = "StatefulAluExpressionDetails"
        description = "Detailed information about a particular expression being executed by a stateful ALU instruction, e.g. ALU hi/lo, condition hi/lo, and output."
        operation = jsl.StringField(required=False, description="The operation being performed.")
        operand_1_type = jsl.StringField(required=False, enum=["phv", "immediate", "register_param", "math", "memory", "condition", "result", "binary", "unary"], description="The type of operand 1.")
        operand_1_value = jsl.OneOfField([jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True),
                                          jsl.StringField()
                                         ], required=False, description="A field name, reference name, immediate value, register parameter name, binary expression, or unary expression.")
        operand_2_type = jsl.StringField(required=False, enum=["phv", "immediate", "register_param", "math", "memory", "condition", "result", "binary", "unary"], description="The type of operand 2.")
        operand_2_value = jsl.OneOfField([jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True),
                                          jsl.StringField()
                                         ], required=False, description="A field name, reference name, immediate value, register parameter name, binary expression, or unary expression.")

    class StatefulAluDetails(jsl.Document):
        title = "StatefulAluDetails"
        description = "Information related to a specific stateful ALU blackbox instruction."

        name = jsl.StringField(required=True, description="The name of the stateful ALU instruction being run (the blackbox name).")
        single_bit_mode = jsl.BooleanField(required=True, description="A Boolean indicating if this expression is operating in single-bit mode.")

        condition_hi = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about condition_hi.")
        condition_lo = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about condition_lo.")

        update_lo_1_predicate = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_lo_1_predicate.")
        update_lo_1_value = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_lo_1_value.")
        update_lo_2_predicate = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_lo_2_predicate.")
        update_lo_2_value = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_lo_2_value.")

        update_hi_1_predicate = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_hi_1_predicate.")
        update_hi_1_value = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_hi_1_value.")
        update_hi_2_predicate = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_hi_2_predicate.")
        update_hi_2_value = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about update_hi_2_value.")

        output_predicate = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about output_predicate.")
        output_value = jsl.DocumentField("StatefulAluExpressionDetails", as_ref=True, required=False, description="Expression details about output_value.")
        output_dst = jsl.StringField(required=False, description="The name of the field to write the output result to.")

        initial_value_hi = jsl.IntField(required=False, description="Initial value to use for memory hi in all table entries (only applicable in dual-width mode).")
        initial_value_lo = jsl.IntField(required=False, description="Initial value to use for memory lo in all table entries.")


    class P4Primitive(jsl.Document):
        title = "P4Primitive"
        description = "Information related to a primitive called from a P4 action."

        name = jsl.StringField(required=True, description="The P4 primitive name.")
        operation = jsl.StringField(required=False, description="A more specific indication of the operation performed.")
        dst = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name of the destination."),
            "type": jsl.StringField(required=True, enum=["phv", "action_param", "immediate", "header", "hash", "counter", "meter", "stateful"], description="The type of the destination."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with the destination for this primitive.")
        dst_mask = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The value of the destination mask."),
            "type": jsl.StringField(required=True, enum=["immediate"], description="A mask indicating which bits of the dst will be modified by this operation."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with the destination mask for this primitive.")
        src1 = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name or value of source 1."),
            "type": jsl.StringField(required=True, enum=["phv", "action_param", "immediate", "header", "hash"], description="The type of source 1."),
            "algorithm": jsl.StringField(required=False, description="The name of the hash algorithm being used."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with source 1 for this primitive.")
        src2 = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name or value of source 2."),
            "type": jsl.StringField(required=True, enum=["phv", "action_param", "immediate", "header", "hash"], description="The type of source 2."),
            "algorithm": jsl.StringField(required=False, description="The name of the hash algorithm being used."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with source 2 for this primitive.")
        src3 = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name or value of source 3."),
            "type": jsl.StringField(required=True, enum=["immediate"], description="The type of source 3."),
            "algorithm": jsl.StringField(required=False, description="The name of the hash algorithm being used."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with source 3 for this primitive.")
        hash_inputs = jsl.ArrayField(items=jsl.StringField(), required=False, description="An array of the field names that participate in a hash computation used in this primitive.")
        idx = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name or value of the source."),
            "algorithm": jsl.StringField(required=False, description="The name of the hash algorithm being used."),
            "type": jsl.StringField(required=True, enum=["phv", "action_param", "immediate", "header", "hash"], description="The type of this index."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with the index used.")
        cond = jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name or value of the condition parameter."),
            "type": jsl.StringField(required=True, enum=["phv", "action_param", "immediate", "header", "hash"], description="The type of this index."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display.")
        }, required=False, description="The properties associated with a condition argument.")
        stateful_alu_details = jsl.DocumentField("StatefulAluDetails", as_ref=True, required=False, description="The properties associated with the stateful ALU instruction being run.")

    title = "MatchTable"
    description = "MatchTable context information."
    table_type = jsl.StringField(required=True, enum=['match'], description="Type of table.")

    uses_range = jsl.BooleanField(required=True, description="Whether this table uses range fields or not.")
    range_entries = jsl.IntField(required=False, description="The number of P4-level range entries available.")
    disable_atomic_modify = jsl.BooleanField(required=False, description="Boolean indicating whether atomic table modifications should be disabled.")

    action_data_table_refs = jsl.ArrayField(jsl.DocumentField("ReferencedTable", as_ref=True), required=True, description="Action Data Tables referenced from this table.")
    selection_table_refs = jsl.ArrayField(jsl.DocumentField("ReferencedTable", as_ref=True), required=True, description="Selection Tables referenced from this table.")
    statistics_table_refs = jsl.ArrayField(jsl.DocumentField("ReferencedTable", as_ref=True), required=True, description="Statistics Tables referenced from this table.")
    meter_table_refs = jsl.ArrayField(jsl.DocumentField("ReferencedTable", as_ref=True), required=True, description="Meter Tables referenced from this table.")
    stateful_table_refs = jsl.ArrayField(jsl.DocumentField("ReferencedTable", as_ref=True), required=True, description="Stateful Tables referenced from this table.")
    # FIXME: These next two fields should be in the stage table instead of here?
    default_next_table_mask = jsl.IntField(required=True, description="Mask used to not overwrite the next_table field in the match spec. FIXME")
    default_next_table_default = jsl.IntField(description="Default to use when updating the next_table_format_data")

    match_key_fields = jsl.ArrayField(items=jsl.DictField(properties={
        "name": jsl.StringField(required=True, description="The fully qualified name of the field."),
        "instance_name": jsl.StringField(required=True, description="The header or metadata instance name for the field."),
        "field_name": jsl.StringField(required=True, description="The name of the field."),
        "match_type": jsl.StringField(required=True, enum=["exact", "lpm", "range", "ternary", "valid"], description="An indication of the match type for this field."),
        "start_bit": jsl.IntField(required=True, description="An integer indicating the start bit, from the least significant bit, to use for the field."),
        "bit_width": jsl.IntField(required=True, description="The number of bits of the field involved in match.  If this is not the entire field, the field slice can be represented as field[start_bit + bit_width - 1 : start_bit]."),
        "bit_width_full": jsl.IntField(required=True, description="The bit width of the field in its entirety.  This will match the bit_width attribute except when it is a field slice."),
        "mask": jsl.StringField(required=False, description="A mask (as hex string to accommodate >64 bits values) of the bits that make up the key. If absent, the full bit width makes up the key. This is not a ternary mask."),
        "position": jsl.IntField(required=False, description="The position in p4_param_order. Repeated fields (for disjointed bit ranges) get the same position."),
        "is_valid": jsl.BooleanField(required=True, description="Whether this match key field is on the valid bit of the specified field."),
        "global_name": jsl.StringField(required=False, description="The optional name of the field used in a global context. This is the name assigned to the snapshot name for the field"),
        "user_annotations": user_annotations_var

    }), required=True, description="An array of match key field properties.")
    actions = jsl.ArrayField(items=jsl.DictField(properties={
        "name": jsl.StringField(required=True, description="The P4 name of the action."),
        "handle": jsl.IntField(required=True, description="A unique identifier for this action."),
        "constant_default_action": jsl.BooleanField(required=True, description="A Boolean indicating if this action is a constant default action.  If true, the action cannot be changed by the the control plane.  The action parameters, if any, can be changed in any scenario."),
        "is_compiler_added_action": jsl.BooleanField(required=True, description="A Boolean indicating if this action was synthesized by the compiler (and likely does not need program-level visibility)."),
        "allowed_as_hit_action": jsl.BooleanField(required=True, description="A Boolean indicating if this action can be used as a table hit action."),
        "allowed_as_default_action": jsl.BooleanField(required=True, description="A Boolean indicating if this action can be used as a table default action (the table miss action)."),
        "disallowed_as_default_action_reason": jsl.StringField(description="If this action cannot be used as a default action, a short description indicating the reason it cannot."),
        "p4_parameters": jsl.ArrayField(items=jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name of the parameter from P4."),
            "start_bit": jsl.IntField(required=True, description="The bit offset of this parameter in the action parameter list. This value refers to the most significant bit."),
            "bit_width": jsl.IntField(required=True, description="The bit width of the parameter."),
            "default_value": jsl.StringField(description="Default value of this parameter (as hex string to accommodate >64 bit values), in case this action is the default one."),
            "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display."),
            "user_annotations": user_annotations_var
        }), required=True, description="An array of P4 parameter properties used by this action."),
        "direct_resources": jsl.ArrayField(items=jsl.DictField(properties={
            "handle": jsl.IntField(required=True, description="Unique identifier of the direct resource."),
            "resource_name": jsl.StringField(required=True, description="The name of the direct resource.")
            }), required=True, description="A list of the direct resources used by this action."),
        "indirect_resources": jsl.ArrayField(items=jsl.AnyOfField([
            jsl.DictField(properties={
                "access_mode": jsl.StringField(required=True, enum=['index'], description="The mode used to access the indirect resource. If this value is 'constant', then the indirect resource is accessed through a constant index that cannot be changed at runtime. If it is 'index', the index comes from action data."),
                "parameter_name": jsl.StringField(description="The action parameter name corresponding to how the resource is accessed. Only applicable if the access mode is 'index'."),
                "parameter_index": jsl.IntField(description="The index of the parameter in the action parameter list."),
                "handle": jsl.IntField(required=True, description="Unique identifier of the indirect resource."),
                "resource_name": jsl.StringField(required=True, description="The name of the indirect resource consumed.")
            }),
            jsl.DictField(properties={
                "access_mode": jsl.StringField(required=True, enum=['constant'], description="The mode used to access the indirect resource. If this value is 'constant', then the indirect resource is accessed through a constant index that cannot be changed at runtime. If it is 'index', the index comes from action data."),
                "value": jsl.IntField(required=True, description="The constant value. Only applicable if the access mode is 'constant'."),
                "handle": jsl.IntField(required=True, description="Unique identifier of the indirect resource."),
                "resource_name": jsl.StringField(required=True, description="The name of the indirect resource consumed.")
            }),
        ]), required=True, description="A list of the indirect resources consumed by this action."),
        "override_stat_addr_pfe": jsl.BooleanField(required=True, description="A Boolean indicating the driver should always write a 1 to the PFE bit in the statistics address in match overhead."),
        "override_meter_addr_pfe": jsl.BooleanField(required=True, description="A Boolean indicating the driver should always write a 1 to the PFE bit in the meter address in match overhead."),
        "override_stateful_addr_pfe": jsl.BooleanField(required=True, description="A Boolean indicating the driver should always write a 1 to the PFE bit in the stateful address in match overhead."),
        "override_stat_addr": jsl.BooleanField(required=True, description="A Boolean indicating the driver should always write the value indicated in override_stat_full_addr to the statistics address in match overhead."),
        "override_meter_addr": jsl.BooleanField(required=True, description="A Boolean indicating the driver should always write the value indicated in override_meter_full_addr to the meter address in match overhead."),
        "override_stateful_addr": jsl.BooleanField(required=True, description="A Boolean indicating the driver should always write the value indicated in override_stateful_full_addr to the stateful address in match overhead."),
        "override_stat_full_addr": jsl.IntField(required=True, description="The full hardware address of the statistics address to encode in the match overhead.  This is a 20-bit value."),
        "override_meter_full_addr": jsl.IntField(required=True, description="The full hardware address of the meter address to encode in the match overhead.  This is a 27-bit value."),
        "override_stateful_full_addr": jsl.IntField(required=True, description="The full hardware address of the stateful address to encode in the match overhead.  This is a 27-bit value."),
        "is_action_meter_color_aware": jsl.BooleanField(required=True, description="A Boolean indicating if this action performs a metering operation that is color aware."),
        "primitives": jsl.ArrayField(items=jsl.DocumentField("P4Primitive", as_ref=True), required=True, description="An list of the primitives called from this action."),
        "user_annotations": user_annotations_var

    }), required=True, description="An array of action properties that are available for this match table.")


    # dynamic_match_key_masks = jsl.BooleanField(description="A Boolean indicating if dynamic match key masks are available for this table.  This is only application for exact match tables.")

    static_entries = jsl.ArrayField(items=jsl.DictField(properties={
        "action_handle": jsl.IntField(required=True, description="The unique handle for the action to run."),
        "priority": jsl.IntField(required=False, description="An integer indicating the entry priority.  Priority 0 is the highest priority.  This field is required if a ternary, lpm, or range match type is used."),
        "is_default_entry": jsl.BooleanField(required=True, description="A Boolean indicating if this is the default table entry."),
        "action_parameters_values": jsl.ArrayField(items=jsl.DictField(properties={
            "parameter_name": jsl.StringField(required=True, description="The name of the action parameter."),
            "value": jsl.StringField(required=True, description="An integer indicating the value (as hex string to accommodate >64 bit values) to program for this parameter.")
        }), required=True, description="An array of the action data parameter properties."),
        "match_key_fields_values": jsl.ArrayField(items=jsl.DictField(properties={
            "field_name": jsl.StringField(required=True, description="The name of the field."),
            "value": jsl.StringField(required=False, description="An integer indicating the value (as hex string to accommodate >64 bit values) to program for this field.  This field is required for all but range fields."),
            "mask": jsl.StringField(required=False, description="For the ternary match type, the mask value (as hex string to accommodate >64 bits values) to use with the field value."),
            "prefix_length": jsl.IntField(required=False, description="For the lpm match type, the prefix length to use with the value."),
            "range_start": jsl.StringField(required=False, description="An integer (as hex string to accommodate >64 bit values) indicating the start value of a range.  A range match occurs if the field value is within [start_range:end_range]."),
            "range_end": jsl.StringField(required=False, description="An integer (as hex string to accommodate >64 bit values) indicating the end value of a range.  A range match occurs if the field value is within [start_range:end_range]."),
            "user_annotations": user_annotations_var
        }), required=True, description="An array of match key field properties for this entry.")
    }, description="An array of the static entries to be programmed by the driver."))

    default_action_handle = jsl.IntField(description="The handle to the P4-specified default action for a table.  This is the action run when the match table misses.  If no default action is specified, no action is taken.")

    action_profile = jsl.StringField(description="Name of the action profile currently in use. This information is used for PD generation.")
    ap_bind_indirect_res_to_match = jsl.ArrayField(jsl.StringField(), description="This value is deduced from the bind_indirect_res_to_match pragma. It is used by PD to indicate that an indirect resource is attached to the match table, and not to the action data as it usually is.")

    is_resource_controllable = jsl.BooleanField(description="A Boolean specifying whether this table is controllable by the driver. This information is used by the PD generation tool.")

    # Attributes specific to each type of MatchTable.
    match_attributes = jsl.OneOfField([
        jsl.DocumentField("ExactMatchTableAttr", as_ref=True),
        jsl.DocumentField("TernMatchTableAttr", as_ref=True),
        jsl.DocumentField("ATCAMMatchTableAttr", as_ref=True),
        jsl.DocumentField("ATCAMMatchUnitAttr", as_ref=True),
        jsl.DocumentField("ALPMMatchTableAttr", as_ref=True),
        jsl.DocumentField("CLPMMatchTableAttr", as_ref=True),
        jsl.DocumentField("MatchWithNoKeyTableAttr", as_ref=True),
        jsl.DocumentField("HashActionTableAttr", as_ref=True),
        jsl.DocumentField("Phase0MatchTableAttr", as_ref=True)
    ], required=True, description="The implementation type for this match table.")

class ActionDataTable(Table):
    title = "ActionDataTable"
    description = "ActionDataTable context information."

    # Require the table to be of this type. This attaches the attributes to the type.
    table_type = jsl.StringField(required=True, enum=['action'], description="Type of table.")
    how_referenced = jsl.StringField(required=True, enum=['direct', 'indirect'], description="Whether the table is directly or indirectly referenced.")

    actions = jsl.ArrayField(items=jsl.DictField(properties={
        "name": jsl.StringField(required=True, description="The P4 name of the action."),
        "handle": jsl.IntField(required=True, description="A unique identifier for this action."),
        "p4_parameters": jsl.ArrayField(items=jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name of the parameter from P4."),
            "start_bit": jsl.IntField(required=True, description="The bit offset of this parameter in the action parameter list. This value refers to the most significant bit."),
            "bit_width": jsl.IntField(required=True, description="The bit width of the parameter."),
            "default_value": jsl.IntField(description="Default value of this parameter, in case this action is the default one."),
            "user_annotations": user_annotations_var
        }), required=True, description="An array of P4 parameter properties used by this action."),
        "user_annotations": user_annotations_var
    }), required=True, description="An array of action properties that are available in this action data table.")

    # dynamic_match_key_masks = jsl.BooleanField(description="A Boolean indicating if dynamic match key masks are available for this table.  This is only application for exact match tables.")

    static_entries = jsl.ArrayField(items=jsl.DictField(properties={
        "match_key": jsl.ArrayField(items=jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name of the field."),
            "match_type": jsl.StringField(required=True, enum=["exact", "lpm", "ternary"], description="The match type to use for the field."),
            "value": jsl.IntField(required=True, description="An integer indicating the value to program for this field."),
            "mask": jsl.IntField(description="An integer indicating the mask value to program for this field.  This is required if the match type is ternary.")
        }), required=True, description="An array of match key field properties for this entry."),
        "priority": jsl.IntField(description="An integer indicating the entry priority.  Not currently used."),
        "default_entry": jsl.BooleanField(required=True, description="A Boolean indicating if this is the default table entry."),
        "action_entry": jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="The name of the action to run."),
            # FIXME: Better name
            "action_id": jsl.IntField(required=True, description="The unique handle for the action to run."),
            "action_data": jsl.ArrayField(items=jsl.DictField(properties={
                "name": jsl.StringField(required=True, description="The name of the action parameter."),
                "value": jsl.IntField(required=True, description="An integer indicating the value to program for this parameter.")
            }), required=True, description="An array of the action data parameter properties.")
        }, required=True, description="An array of action properties for this entry.")
    }, description="An array of the static entries to be programmed by the driver."))

    stage_tables = jsl.ArrayField(items=jsl.DocumentField("ActionDataStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this action data table.")

class SelectionTable(Table):
    title = "SelectionTable"
    description = "SelectionTable context information."
    table_type = jsl.StringField(required=True, enum=['selection'], description="Type of table.")

    selector_name = jsl.StringField(required=True, description="The name given to the action selector in P4.")
    selection_type = jsl.StringField(required=True, enum=['resilient', 'fair'], description="Whether the selection is [resilient] or [fair].")
    selection_key_name = jsl.StringField(required=True, description="The name given to the selector calculation object in P4.")
    dynamic_selection_calculation = jsl.StringField(description="If this selector uses dynamic hashing, the name given to the P4 calculation object.")
    bound_to_action_data_table_handle = jsl.IntField(description="An integer indicating the action data table handle this is bound to, if any. FIXME: Update description.")
    bound_to_stateful_table_handle = jsl.IntField(description="An integer indicating the stateful table handle this is bound to, if any.  A selection table is bound to a stateful table in cases where a packet generator packet is used to update the live port members.")
    how_referenced = jsl.StringField(required=True, enum=['direct', 'indirect'], description="Whether the table is directly or indirectly referenced.")

    max_port_pool_size = jsl.IntField(description="An integer indicating the maximum number of members available in any selection group.")

    stage_tables = jsl.ArrayField(items=jsl.DocumentField("SelectionStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this selection table.")

class StatisticsTable(Table):
    title = "StatisticsTable"
    description = "StatisticsTable context information."
    table_type = jsl.StringField(required=True, enum=['statistics'], description="Type of table.")

    statistics_type = jsl.StringField(required=True, enum=['packets', 'bytes', 'packets_and_bytes'], description="Type of statistics that are being gathered, [packets], [bytes] or [packets_and_bytes].")
    how_referenced = jsl.StringField(required=True, enum=['direct', 'indirect'], description="Whether the table is directly or indirectly referenced.")
    enable_pfe = jsl.BooleanField(required=True, description="A Boolean indicating whether the per-flow enable (PFE) bit needs to be included in the statistics address in the match table overhead.")
    pfe_bit_position = jsl.IntField(required=True, description="An integer indicating the bit position in the statistics address field in match overhead, from the least significant bit position, of the PFE bit.")
    packet_counter_resolution = jsl.IntField(required=True, description="FIXME")
    byte_counter_resolution = jsl.IntField(required=True, description="FIXME")

    stage_tables = jsl.ArrayField(items=jsl.DocumentField("StatisticsStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this statistics table.")

class MeterTable(Table):
    title = "MeterTable"
    description = "MeterTable context information."
    table_type = jsl.StringField(required=True, enum=['meter'], description="Type of table.")

    meter_type = jsl.StringField(required=True, enum=['standard', 'lpf', 'red'], description="Type of meter in use.")
    meter_profile = jsl.IntField(required=False, enum=list(range(0, 16)), description="For color-based meters, a profile specifies what minimum and maximum burst rates are supported.  The default value is 0.")
    meter_granularity = jsl.StringField(required=True, enum=['bytes', 'packets'], description="Granularity of meter in use.")
    how_referenced = jsl.StringField(required=True, enum=['direct', 'indirect'], description="Whether the table is directly or indirectly referenced.")
    enable_pfe = jsl.BooleanField(required=True, description="A Boolean indicating whether the per-flow enable (PFE) bit needs to be included in the meter address in the match table overhead.")
    pfe_bit_position = jsl.IntField(required=True, description="An integer indicating the bit position in the meter address field in match overhead, from the least significant bit position, of the PFE bit.")
    enable_color_aware_pfe = jsl.BooleanField(required=True, description="A Boolean indicating whether the meter address type needs to be included in the meter address in the match table overhead.  This arises if some match table actions use pre-color and some do not.")
    color_aware_pfe_address_type_bit_position = jsl.IntField(required=True, description="An integer indicating the bit position in the meter address field in match overhead, from the least significant bit position, of the start of the meter address type (3 bits).")
    pre_color_field_name = jsl.StringField(description="The name of a pre-color field object used by this meter, if any.")

    reference_dictionary = jsl.DictField(additional_items=True, required=True, description="Dictionary that maps match table names to how it references this meter table. FIXME: This could be removed from future context JSON implementations.")

    stage_tables = jsl.ArrayField(items=jsl.DocumentField("MeterStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this meter table.")

class StatefulTable(Table):
    title = "StatefulTable"
    description = "StatefulTable context information."
    table_type = jsl.StringField(required=True, enum=['stateful'], description="Type of table.")

    how_referenced = jsl.StringField(required=True, enum=['direct', 'indirect'], description="Whether the table is directly or indirectly referenced.")

    # FIXME: These things are dependent on the width mode...
    dual_width_mode = jsl.BooleanField(required=True, description="A Boolean indicating if this stateful table operates in dual-width mode.")
    # NOTE: alu_width = [64, 128] only supported in JBAY
    alu_width = jsl.IntField(required=True, enum=[1, 8, 16, 32, 64, 128], description="An integer indicating the stateful ALU data path width.  Normally, this corresponds to the memory entry width, but for dual-width mode, the memory entry width is twice this value. ")
    set_instr_adjust_total = jsl.IntField(enum=[0, 1, 2, 3], description="An integer indicating which stateful ALU instruction number contains a set bit adjust total instruction.  This is only applicable in single bit mode.  The driver uses this when creating the stateful ALU address.")
    set_instr = jsl.IntField(enum=[0, 1, 2, 3], description="An integer indicating which stateful ALU instruction number contains a set bit instruction.  This is only applicable in single bit mode.  The driver uses this when creating the stateful ALU address.")
    clr_instr_adjust_total = jsl.IntField(enum=[0, 1, 2, 3], description="An integer indicating which stateful ALU instruction number contains a clear bit adjust total instruction.  This is only applicable in single bit mode.  The driver uses this when creating the stateful ALU address.")
    clr_instr = jsl.IntField(enum=[0, 1, 2, 3], description="An integer indicating which stateful ALU instruction number contains a clear bit instruction.  This is only applicable in single bit mode.  The driver uses this when creating the stateful ALU address.")
    initial_value_lo = jsl.NumberField(required=True, description="An integer indicating the initial value to use for the stateful word.  If the stateful table is not in dual-width mode, this is the initial value for the entire stateful word.  If no initial value was specified, 0 will be used.")
    initial_value_hi = jsl.NumberField(description="An integer indicating the initial value to use for the most significant half of the stateful word.  This is only valid in double-width mode.  If no initial value was specified, 0 will be used.")
    bound_to_selection_table_handle = jsl.IntField(description="An integer indicating the selection table handle this is bound to, if any.  A stateful table is bound to a selector in cases where a packet generator packet is used to update the live port members.")
    stateful_table_type = jsl.StringField(required=False, enum=['normal', 'log', 'fifo', 'stack', 'bloom_clear'], description="Type of stateful table.")
    stateful_direction = jsl.StringField(required=False, enum=['in', 'out', 'inout'], description="Direction of dataplane accesses to a fifo or stack")
    stateful_counter_index = jsl.IntField(required=False, enum=list(range(0, 4)), description="An integer that indicates the counter set used for this stateful log, fifo, or stack")

    action_to_stateful_instruction_slot = jsl.ArrayField(items=jsl.DictField(properties={
            "action_handle": jsl.IntField(required=True, description="Unique identifier for an action."),
            "instruction_slot": jsl.IntField(required=True, description="Stateful ALU instruction slot used by the action.")
        }, description="A dictionary that maps action handles to the stateful ALU instruction slot used by that action, if any.  This is used by the driver to help encode the stateful ALU address."),
        required=True,
        description="List of dictionaries containing action handles and their corresponding stateful ALU instruction, if any.")

    register_params = jsl.ArrayField(items=jsl.DictField(properties={
        "register_file_index": jsl.IntField(required=True, description="Index of the SALU register file row the register parameter is stored in."),
        "initial_value": jsl.IntField(required=True, description="Initial value of the register parameter."),
        "name": jsl.StringField(required=True, description="Name of the register parameter."),
        "handle": jsl.IntField(required=True, description="Unique identifier of the register parameter.")
    }), required=False, description="List of register parameters stored in the SALU register file.")

    stage_tables = jsl.ArrayField(items=jsl.DocumentField("StatefulStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this stateful table.")

class ConditionTable(Table):
    title = "ConditionTable"
    description = "ConditionTable context information."
    table_type = jsl.StringField(required=True, enum=['condition'], description="Type of table.")

    condition = jsl.StringField(required=True, description="String representation of condition from P4.")
    attached_to = jsl.OneOfField([jsl.NullField(), jsl.StringField()], required=True, description="The name of a match table this condition table is attached to, or null if unattached.")
    stage_tables = jsl.ArrayField(items=jsl.DocumentField("GatewayStageTable", as_ref=True), required=True, description="An array of stage-specific resource objects for this condition table.")
    condition_fields = jsl.ArrayField(items=jsl.DictField(properties={
        "name": jsl.StringField(required=True, description="The fully qualified name of the field."),
        "start_bit": jsl.IntField(required=True, description="An integer indicating the start bit, from the least significant bit, to use for the field."),
        "bit_width": jsl.IntField(required=True, description="The number of bits of the field involved in the condition.  If this is not the entire field, the field slice can be represented as field[start_bit + bit_width - 1 : start_bit].")
    }), required=True, description="An array of fields used by this condition.")



#### PARSER ####
class PVS(jsl.Document):
    title = "PVS"
    description = "Parser value set context information."

    parser_name = jsl.StringField(required=True, description="P4 parser state name.")
    parser_state_id = jsl.IntField(required=True, description="Compiler-assigned parser state ID.  Allowed values are 0 to 255.")
    uses_pvs = jsl.BooleanField(required=True, description="Boolean indicating if this parser state uses a parser value set.")
    pvs_name = jsl.StringField(description="Name of this PVS.")
    pvs_handle = jsl.IntField(description="Unique identifier for the parser value set.")

    tcam_rows = jsl.ArrayField(jsl.IntField(), required=True, description="An array of parser TCAM rows that are used by this parse state.")
    match_registers = jsl.ArrayField(items=jsl.DictField(properties={
        # Note: container_hardware_id valid enums [0, 2, 3] = Tofino, [0, 1, 2, 3] = Tofino2
        "container_hardware_id": jsl.IntField(required=True, enum=[0, 1, 2, 3], description="The hardware ID of a parser match extractor.  Allowed values correspond to the byte in the 32-bit match window.  ID 0 is the 16-bit match register.  ID 2 is the first 8-bit match register, and ID 3 is the second 8-bit match register."),
        "container_width": jsl.IntField(required=True, enum=[8, 16], description="The bit width of the match register.  ID 0 must have a value of 16.  IDs 2 and 3 must have a value of 8."),
        "mask": jsl.IntField(required=True, description="An integer containing the mask corresponding to which bits are matched against from the P4 program, and which ones are don't care."),
        "field_mapping": jsl.ArrayField(items=jsl.DictField(properties={
            "field_name": jsl.StringField(required=True, description="The name of the field."),
            "start_bit": jsl.IntField(required=True, description="A particular bit of the field, indexed from the least significant position."),
            "register_bit": jsl.IntField(required=True, description="The register bit in the parser match register, indexed from the least significant position."),
            "select_statement_bit": jsl.IntField(required=True, description="The select bit position from the P4 select statement, indexed from the least significant position.")
        }), required=True, description="List of parser select statement mapping into this match register")
    }, description="Parser match register properties."), required=True, description="An array of parser match registers in use by this parser value set.")

### schema for single parser programming
class Parser(jsl.Document):
    title = "Parser"
    description = "Parser context information."

    ingress = jsl.ArrayField(jsl.DocumentField("PVS", as_ref=True), required=True, description="Parser value set information for Ingress.")
    egress = jsl.ArrayField(jsl.DocumentField("PVS", as_ref=True), required=True, description="Parser value set information for Egress.")

### schema for multiple parser programming
class Parsers(jsl.Document):
    class ParserInstance(jsl.Document) :
        title = "ParserInstance"
        description = "Parser instance context information."
        name = jsl.StringField(required=True, description="Name of the parser instance.")
        handle = jsl.IntField(required=True, description="Unique handle to identify a parser program instance.")
        states = jsl.ArrayField(jsl.DocumentField("PVS", as_ref=True), required=True, description="Parser state information.")
        default_parser_id = jsl.ArrayField(jsl.IntField(required=True, description="Default parser id information."))

    title = "Parsers"
    description = "Multiple parsers context information."
    ingress = jsl.ArrayField(jsl.DocumentField("ParserInstance", as_ref=True), required=True, description="Parser instances information for Ingress.")
    egress = jsl.ArrayField(jsl.DocumentField("ParserInstance", as_ref=True), required=True, description="Parser instances information for Egress.")

#### PHV ALLOCATION ####

class PHVStageAllocation(jsl.Document):
    class PHV(jsl.Document):
        title = "PHV"
        description = "PHV context information."

        phv_number = jsl.IntField(required=True, description="Unique address of this PHV container.")
        container_type = jsl.StringField(required=True, enum=["normal", "tagalong", "mocha", "dark"], description="A string describing the type of this PHV container.")
        word_bit_width = jsl.IntField(required=True, enum=[8, 16, 32], description="The PHV container bit width.")
        records = jsl.ArrayField(items=jsl.DictField(
            properties={
                "is_pov": jsl.BooleanField(required=True, description="A Boolean indicating whether this field is a POV."),
                "field_name": jsl.StringField(required=True, description="Name of the field."),
                "field_lsb": jsl.IntField(required=True, description="Least significant bit of the field in this container."),
                "field_msb": jsl.IntField(required=True, description="Most significant bit of this field in this container.  This container will hold field[msb:lsb]."),
                "phv_lsb": jsl.IntField(required=True, description="Least significant bit of this PHV container that is occupied."),
                "phv_msb": jsl.IntField(required=True, description="Most significant bit of this PHV container that is occupied.  This allocation is phv[phv_msb:phv_lsb] = field[field_msb:field_lsb]."),
                "is_compiler_generated": jsl.BooleanField(required=True, description="Indicates whether this field was generated by the compiler."),
                "pov_headers": jsl.ArrayField(items=jsl.DictField(properties={
                    "header_name": jsl.StringField(required=True, description="Name of the POV header."),
                    "bit_index": jsl.IntField(required=True, description="The index of this POV bit in the PHV record."),
                    "position_offset": jsl.IntField(required=True, description="The position offset of this field in what would correspond to the driver's byte array. The driver puts all fields in an ordered fashion from each PHV into a byte array. Fields that are 3 byte long are padded to 4 bytes."),
                    "hidden": jsl.BooleanField(required=True, description="Whether this field is hidden from the driver's byte array.")
                }), description="An array of the POV headers in this PHV record."),
                "field_width": jsl.IntField(required=True, description="Full width of this field in the P4 program. The field may be split among multiple PHVs."),
                "position_offset": jsl.IntField(required=True, description="The position offset of this field in what would correspond to the driver's byte array. The driver puts all fields in an ordered fashion from each PHV into a byte array. Fields that are 3 byte long are padded to 4 bytes."),
                "live_start": jsl.OneOfField([jsl.IntField(),jsl.StringField(enum=["parser", "deparser"])],
                                             required=False, description="The location this field is first live."),
                "live_end": jsl.OneOfField([jsl.IntField(),jsl.StringField(enum=["parser", "deparser"])],
                                             required=False, description="The location this field is last live."),
                "mutually_exclusive_with": jsl.ArrayField(items=jsl.StringField(required=True, description="Name of a field also found in this container that the field is mutually exclusive with."),
                                                          required=False, description="List of field names in this container that this field is mutually exclusive with."),
                "format_type": jsl.StringField(required=False, description="A user-supplied type for this field, which can be used for string formatting during display."),
                "tcam_rows": jsl.ArrayField(items=jsl.IntField(required=True), required=False, description="A list of parser TCAM rows this field is extracted on.")
            }, description="Description of a field occupying this PHV."),
            required=True,
            description="List of fields occupying this PHV container.")

    title = "PHVStageAllocation"
    description = "PHVStageAllocation info."

    stage_number = jsl.IntField(required=True, description="Number of the stage attached to this PHV.")
    ingress = jsl.ArrayField(jsl.DocumentField("PHV", as_ref=True), required=True)
    egress = jsl.ArrayField(jsl.DocumentField("PHV", as_ref=True), required=True)

class LearnQuanta(jsl.Document):
    title = "LearnQuanta"
    description = "LearnQuanta context information."

    name = jsl.StringField(required=True, description="Name of the field list in the learn quanta.")
    handle = jsl.IntField(required=True, description="Unique identifier to this learn quanta.")
    lq_cfg_type = jsl.IntField(required=True, description="An int representing the unique hardware ID that has been given to this field list, if used for hardware learning (generate digest).")
    fields = jsl.ArrayField(jsl.DictField(properties={
        "field_name": jsl.StringField(required=True, description="Name of this field."),
        "phv_offset": jsl.IntField(required=False, description="p4 field offset of this phv slice"),
        "field_width": jsl.IntField(required=True, description="Bit width of this phv slice"),
        "start_bit": jsl.IntField(required=True, description="FIXME The start bit of the field from which the learner will take information from."),
        "start_byte": jsl.IntField(required=True, description="FIXME The start byte of the field from which the learner will take information from.")
    }, description="Dictionary containing the field name and its width."),
    required=True,
    description="List of fields in this learn quanta.")


#### MAU Stage Characteristics ####
class MauStageCharacteristics(jsl.Document):
    title = "MauStageCharacteristics"
    description = "MauStageCharacteristics info."

    stage = jsl.IntField(required=True, description="Stage id.")
    gress = jsl.StringField(required=True, enum=["ingress", "egress", "ghost"], description="The gress this stage belongs to.")
    match_dependent = jsl.BooleanField(required=True, description="Boolean indicating if this stage is match dependent on the previous stage.")
    clock_cycles = jsl.IntField(required=True, desciption="The number of clock cycles this stage requires to complete all processing.")
    predication_cycle = jsl.IntField(required=True, desciption="The clock cycle in which predication occurs.")
    cycles_contribute_to_latency = jsl.IntField(required=True, desciption="The number of clock cycles this stage contributes to the overall pipeline gress latency.")


#### Dynamic Hash Calculation ####
# This is the one to be used with the dynamic library in bf-utils.
####

class DynamicHashCalculation(jsl.Document):

    title = "DynamicHashCalculation"
    description = "DynamicHashCalculation context information."

    class HashConfiguration(jsl.Document):
        title = "HashConfiguration"
        description = "Context information containing the allocation information for hashing resources in each stage."

        stage_number = jsl.IntField(required=True, description="Number identifying this stage.")

        hash = jsl.DictField(properties={
            "hash_id": jsl.IntField(required=True, enum=list(range(0, 8)), description="The hardware hash function ID in use."),
            "num_hash_bits": jsl.IntField(required=True, enum=list(range(0, 52)), description="The number of hash bits in use."),
            "hash_bits": jsl.ArrayField(items=jsl.DictField(properties={
                "gfm_hash_bit": jsl.IntField(required=True, enum=list(range(0, 52)), description="Integer that indicates the specific hash bit being configured (GFM column)."),
                # Note: We only support up to two hardware hash functions, so the maximum number of P4 hash function bits is 104.
                "p4_hash_bit": jsl.IntField(required=True, enum=list(range(0, 104)), description="Integer that indicates the specific hash bit in the user hash function.")
            }), required=True, description="A list of the hardware hash bits in use in the hash function, with their mapping to the original user hash function bit."),
        }, required=True, description="Hardware hash function properties for this stage.")

        # For wide hash functions, (e.g. selector mod, multiple hardware hash functions and crossbar allocations are required.)
        # FIXME(mea): We could extend this to be arbitrarily large at some point, if we want to allow dynamic hash calculations
        # that use more than two hardware hash functions.
        hash_mod = hash

    class CrossbarConfiguration(jsl.Document):
        title = "CrossbarConfiguration"
        description = "Context information containing the allocation information for input crossbar resources in each stage."

        stage_number = jsl.IntField(required=True, description="Number identifying this stage.")

        # TODO: What if duplicate fields in CRC or identity field list?

        crossbar = jsl.ArrayField(items=jsl.DictField(properties={
            "byte_number": jsl.IntField(required=True, enum=list(range(0, 128)),
                                        description="Hardware crossbar byte this field bit is located in."),
            "bit_in_byte": jsl.IntField(required=True, enum=list(range(0, 8)),
                                        description="The bit in the crossbar byte where the field bit is located.  Bit 0 is the least significant bit of the byte."),
            "name": jsl.StringField(required=True, description="The name of the field."),
            "field_bit": jsl.IntField(required=True, description="The bit of the field.")
        }), required=True, description="Crossbar bit-in-byte mapping to field bit.")

        crossbar_mod = crossbar


    name = jsl.StringField(required=True, description="Name of the hash calculation.")
    handle = jsl.IntField(required=True, description="Unique identifier for this hash calculation.")

    field_lists = jsl.ArrayField(jsl.DictField(properties={
        "name": jsl.StringField(required=True, description="Field list name."),
        "handle": jsl.IntField(required=True, description="Unique identifier for this field list."),
        "is_default": jsl.BooleanField(required=True, description="Boolean indicating if this field list is the default field list."),
        "can_permute": jsl.BooleanField(required=False, description="Boolean indicating if the fields in this field list are allowed to be permuted."),
        "can_rotate": jsl.BooleanField(required=False, description="Boolean indicating if the fields in this field list are allowed to be rotated."),

        "fields": jsl.ArrayField(items=jsl.DictField(properties={
            "name": jsl.StringField(required=True, description="Field name."),
            "start_bit": jsl.IntField(required=True, description="The start bit of the field, from the least significant bit position."),
            "bit_width": jsl.IntField(required=True, description="The bit width of this field.  Field slices are represented as name[start_bit + bit_width - 1: start_bit]."),
            "optional": jsl.BooleanField(required=True, description="A Boolean indicating if this field (or field slice) is optional."),
            "is_constant": jsl.BooleanField(required=True, description="A Boolean indicating if this field represents a constant value."),
            "value": jsl.IntField(required=False, description="An integer representing the value of this 'field,' if is_constant is true.")
        }), required=True, description="Ordered list of fields, as specified by the P4 source.  Fields that will become the more significant section of the hash key will appear in lower indices (index 0 is the most significant field)."),

        "crossbar_configuration": jsl.ArrayField(items=jsl.DocumentField("CrossbarConfiguration", as_ref=True, description="Configuration information for the input crossbar hardware mapping for this field list."),
                                                 required=True, description="Per-stage array of crossbar configuration information.")

    }), required=True, description="Available field lists for dynamic hash calculation.")

    any_hash_algorithm_allowed = jsl.BooleanField(required=False, description="A Boolean indicating if any hash algorithm (of the appropriate size) is supported.")
    hash_bit_width = jsl.IntField(required=True, description="The bit width of the hash being computed.  Note that this need not match the width what the hash algorithm can uniquely produce.")

    algorithms = jsl.ArrayField(items=jsl.DictField(properties={
        "name": jsl.StringField(required=True, description="The name of the hash algorithm."),
        "type": jsl.StringField(required=True, enum=["crc", "identity", "random", "xor"], description="The type of the hash algorithm."),
        "handle": jsl.IntField(required=True, description="Unique identifier for this hash algorithm."),
        "is_default": jsl.BooleanField(required=True, description="Boolean indicating if this hash algorithm is the default hash algorithm."),
        "msb": jsl.BooleanField(required=True, description="Boolean indicating if the most significant bits of the computed hash should be used."),
        "extend": jsl.BooleanField(required=True, description="Boolean indicating if the computed hash should be repeated until the output width is filled."),
        "poly": jsl.StringField(required=False, description="For CRC-based hash algorithms, the polynomial it uses."),
        "init": jsl.StringField(required=False, description="For CRC-based hash algorithms, the initial value used to start the CRC calculation. This initial value should be the initial shift register value, reversed if it uses a reversed algorithm, and then XORed with the final XOR value."),
        "reverse": jsl.BooleanField(required=False, description="For CRC-based hash algorithms, a Boolean indicating if a bit-reversed algorithm is used."),
        "final_xor": jsl.StringField(required=False, description="For CRC-based hash algorithms, the final value to XOR with the calculated CRC value.")
    }), required=True, description="Ordered list of fields, as specified by the P4 source.  Fields that will become the more significant section of the hash key will appear in lower indices (index 0 is the most significant field).")

    hash_configuration = jsl.ArrayField(items=jsl.DocumentField("HashConfiguration", as_ref=True, description="Configuration information for the hashing hardware mapping."),
                                        required=True, description="Per-stage array of hash configuration information.")

#### Configuration Cache ####

class ConfigurationValue(jsl.Document):
    title = "ConfigurationValue"
    description = "A specific device register configuration value programmed by the compiler but also needed by the driver."
    name = jsl.StringField(required=True, description="Unique name given, preferably device agnostic.")
    fully_qualified_name = jsl.StringField(description="Fully qualified name in the register hierarchy.")
    value = jsl.StringField(required=True, description="A hexadecimal string representation of the configuration value.")

#### Manual Egress Packet Length Adjustment ####

class EgressPacketLengthAdjust(jsl.Document):
    title = "EgressPacketLengthAdjust"
    description = "A program-specific set of values needed to manually adjust the egress intrinsic metadata packet length."
    field_name = jsl.StringField(required=True, description="The metadata field name to be adjusted.")
    default_adjustment_value = jsl.IntField(required=True, description="An integer indicating the value to adjust the field by in the default case.  This value is a negative value represented in twos complement, so it should be added to the field to adjust.")

    adjustment_values = jsl.ArrayField(items=jsl.DictField(properties={
        "adjustment_value": jsl.IntField(required=True, description="An integer indicating the adjustment value required for this combination.  This value is a negative value represented in twos complement, so it should be added to the field to adjust."),
        "match_key_fields_values": jsl.ArrayField(items=jsl.DictField(properties={
            "field_name": jsl.StringField(required=True, description="The name of the field."),
            "value": jsl.IntField(required=True, description="An integer indicating the value this field has for the given adjustment."),
        }), required=True, description="An array of field properties that must be satisfied to apply this adjustment value.")
    }, description="An array of adjustment values for specific field value combinations."))


### Field information (as slices)
class FieldInfo(jsl.Document):
    title = "FieldInfo"
    description = "Defines a field, or a field slice."
    name = jsl.StringField(required=True, description="The field (slice) name")
    slice = jsl.DictField(required = True,
                          properties = {
        "start_bit": jsl.IntField(required=True,
                                  description="""The least significant bit of the slice.
                                              Must be 0 if this is a full field."""),
        "bit_width": jsl.IntField(required=True, description="Width of field or field slice"),
        "slice_name": jsl.StringField(required=False,
                                      description="""Name of the slice, in case the compiler
                                                  generates a new name.""")
    })

### Header type information
class HeaderType(jsl.Document):
    title = "HeaderType"
    description = "Define header layout: lists the field slices of a header in wire order."
    name = jsl.StringField(required=True, description="Header type name.")
    fields = jsl.ArrayField(required=True,
                            description="""Ordered list of fields or field slices.
                                Note that a field might be sliced and multiple slices will occur
                                in the list with the same field name. They can be distinguished by
                                the slice_name (if present) or the start bit.""",
                            items=jsl.DocumentField("FieldInfo", as_ref=True), min_items=1)

### MAU stage extension information
class MauStageExtension(jsl.Document):
    title = "MauStageExtension"
    description = "Programming for additional stages to load program on a larger chip"
    last_programmed_stage = jsl.IntField(required=True, description="last stage in binary")
    registers = jsl.ArrayField(required=True, description="registers to set in each stage",
        items=jsl.DictField(required=True, properties = {
            "name": jsl.StringField(required=True, description="register name"),
            "offset": jsl.IntField(required=True, description="offset from stage base address"),
            "value": jsl.ArrayField(items=jsl.IntField(), required=True, description="register value(s)"),
            "mask": jsl.ArrayField(items=jsl.IntField(), min_items=3, max_items=3, required=True,
                                   description="masks for old last/intermediate/new last stages")
        }))

class ContextJSONSchema(jsl.Document):
    title = "ContextJSONSchema"
    description = "ContextJSON for a P4 program compiled to the Tofino backend."
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    build_date = jsl.StringField(required=True, description="Timestamp of when the program was built.")
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    compile_command = jsl.StringField(required=False, description="The command line arguments used to compile the program.")
    compiler_version = jsl.StringField(required=True, description="Compiler version used in compilation.")
    schema_version = jsl.StringField(required=True, description="Schema version used to produce this JSON.")
    target = jsl.StringField(required=True, enum=TARGETS,
                             description="The target device this program was compiled for.")

    # Match, Selection, Statistics, ActionData, Meter, Stateful, Condition.
    tables = jsl.ArrayField(items=jsl.AnyOfField([
        jsl.DocumentField("MatchTable", as_ref=True),
        jsl.DocumentField("SelectionTable", as_ref=True),
        jsl.DocumentField("StatisticsTable", as_ref=True),
        jsl.DocumentField("ActionDataTable", as_ref=True),
        jsl.DocumentField("MeterTable", as_ref=True),
        jsl.DocumentField("StatefulTable", as_ref=True),
        jsl.DocumentField("ConditionTable", as_ref=True)
    ], description="Possible types of tables in this program."), required=True, description="List of tables and their specifications in this P4 program.")

    phv_allocation = jsl.ArrayField(items=jsl.DocumentField("PHVStageAllocation", as_ref=True), required=True, description="PHV allocation context for a given stage.")
    parser = jsl.DocumentField("Parser", as_ref=True, description="Parser context information.")
    parsers = jsl.DocumentField("Parsers", required=False, as_ref=True, description="Multiple parsers context information.")
    learn_quanta = jsl.ArrayField(items=jsl.DocumentField("LearnQuanta", as_ref=True), required=True, description="LearnQuanta context information.")
    dynamic_hash_calculations = jsl.ArrayField(items=jsl.DocumentField("DynamicHashCalculation", as_ref=True), required=True, description="Dynamic hash calculation context information.")
    configuration_cache = jsl.ArrayField(items=jsl.DocumentField("ConfigurationValue", as_ref=True), required=True, description="Cached configuration values.")
    egress_packet_length_adjust = jsl.DocumentField("EgressPacketLengthAdjust", as_ref=True, description="Information needed to manually adjust the egress intrinsic metadata packet length.")
    driver_options = jsl.DictField(properties={
        "hash_parity_enabled": jsl.BooleanField(required=True, description="Boolean indicating if hash parity errors will generate interrupts."),
        "high_availability_enabled": jsl.BooleanField(required=False, description="Boolean indicating if the compiler allocated resources to support high availability."),
        "tof1_egr_parse_depth_checks_disabled": jsl.BooleanField(required=False, description="Boolean indicating that Tofino 1 egress parse depth checks were disabled during compilation."),
    }, additional_items=False, required=True, description="Supplemental information required by the driver.")
    mau_stage_characteristics = jsl.ArrayField(items=jsl.DocumentField("MauStageCharacteristics", as_ref=True), required=True,
                                               description="Characteristics information about how a MAU stage has been configured.")
    flexible_headers = jsl.ArrayField(items=jsl.DocumentField("HeaderType", as_ref=True),
                                      required = False,
                                      description = "compiler defined layouts for flexible headers")
    mau_stage_extension = jsl.DocumentField("MauStageExtension", as_ref=True, required=False,
        description="Trailing stage fill information")

#### DOCS GEN ####

def gen_docs(klass):
    """
    Very, very simple doc generation routine that trims out irrelevant things
    such as refs and some confusing JSON Schema notation to something a little
    bit simpler. Can be used with a JSON visualizer to have some nice documentation!
    """
    def should_skip(name):
        return (not name) or (name[0] == '_') or name == 'title' or name == 'description'

    def get_item_docs(item, docs):
        info = None

        if isinstance(item, jsl.ArrayField):
            info = handle_array(item, docs)
        elif isinstance(item, jsl.DictField):
            info = handle_dict(item, docs)
        elif isinstance(item, jsl.DocumentField):
            handle_doc(item.document_cls, docs)
            info = {
                "type": item.document_cls.title,
                "required": item.required
            }
        elif isinstance(item, jsl.StringField):
            info = {
                "type": "string",
                "description": item.description,
                "required": item.required
            }
        elif isinstance(item, jsl.IntField):
            info = {
                "type": "integer",
                "description": item.description,
                "required": item.required
            }
        elif isinstance(item, jsl.BooleanField):
            info = {
                "type": "boolean",
                "description": item.description,
                "required": item.required
            }
        elif isinstance(item, jsl.fields.compound.BaseOfField):
            info = {
                "type": "baseOf",
                "description": item.description,
                "options": [get_item_docs(field, docs) for field in item.fields],
                "required": item.required
            }
        if info and hasattr(item, "_enum") and item._enum:
            info["enum"] = item._enum
        return info

    def handle_array(array, docs):
        # currently not accepting lists as items.
        assert isinstance(array, jsl.ArrayField)
        assert not isinstance(array.items, list)

        info = {
            "type": "array",
            "description": array.description,
            "items": get_item_docs(array.items, docs),
            "required": array.required
        }
        return info

    def handle_dict(d, docs):
        assert isinstance(d, jsl.DictField)
        info = {
            "type": "object",
            "description": d.description,
            "properties": {},
            "required": d.required
        }
        props = info["properties"]
        if d.properties:
            for prop_name, prop in d.properties.items():
                assert prop_name not in props
                props[prop_name] = get_item_docs(prop, docs)
        return info

    def handle_doc(doc, docs):
        assert issubclass(doc, jsl.Document)
        title = doc.title
        if title in docs:
            return

        docs[title] = {
            "type": "object",
            "description": doc.description,
            "properties": {}
        }
        props = docs[title]["properties"]
        for prop_name, prop in inspect.getmembers(doc):
            if isinstance(prop, collections.Callable) or should_skip(prop_name):
                continue
            assert prop_name not in props
            props[prop_name] = get_item_docs(prop, docs)

    docs = {}
    try:
        handle_doc(klass, docs)
    except:
        return None
    return docs

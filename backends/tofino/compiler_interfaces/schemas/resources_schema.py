#!/usr/bin/env python3

"""
resources_schema.py: Generates a JSON Schema model for structured resource allocation result information.
"""

import inspect
import json

import jsl

########################################################
#   Schema Version
########################################################

"""
Version Notes:

1.0.0:
- Initial release
1.0.1:
- Modify parser schema to publish previous_state_id from a parse_state, rather than next_state.
- Allow parser state IDs to be null, to avoid ambiguity about where source/sink state has
  previous/next state.
1.0.2:
- Add exm_search_buses, exm_result_buses, tind_result_buses
1.0.3:
- Change hierarchy location for mau_stages to be under new top-level mau attribute.
- Add nStages under mau, which is the number of stages the program was compiled for.
1.0.4:
- Define CLOT resource usage.
1.0.5:
- Add deparser resource usage information (field dictionary, mirror tables, digest tables, resubmit tables).
1.1.0:
- Add multiple phase0 support. Move phase0 node into parser node
1.1.1:
- Modify clot length range to be between 1 and 65
1.1.2:
- Make clots node an array per gress
1.2.0:
- Changes for parse state transition visualization
    - Rename ParserStateResourceUsage to ParserStateTransitionResourceUsage
      since the node actually represents a state transition
    - Rename state_id & state_name to next_state_id & next_state_name
    - Remove value & mask as Int fields in 'matchesOn' node and replace by value as a String field
      to represent a masked value. e.g. "0x8***01"
    - Remove unused buffer offset field in 'matchesOn' node
    - Add a 'savesTo' node which indicates if a byte or half word (denoted by
      the hardware_id) is saved for matching on the next state. The buffer
      offset indicates the position in the input buffer where the byte/half word
      is saved from
1.3.0:
- Changes for CLOT-related metrics
    - Add a 'clot_eligible_fields' node to indicate the CLOT and PHV allocation for CLOT-eligible fields
1.4.0:
- Change CLOT field lists from string to object to indicate field position in CLOT
1.4.1:
- Add 'way' information to RAM Usage
1.5.0
- Add CLOT tag to deparser chunk
1.5.1:
- Changed next/previous_state_id to just jsl.IntField (previously allowed Null as well, but was not used)
2.0.0
- Removed useless pipes array and pipe_id (can be inferred by p4i)
- Removed CLOT offset from deparser chunk
- Changes CLOT eligible field to required
2.1.0
- changed ClotUsage::issue_state to an array
- flattened ClotUsage::field_lists to 1D array
2.2.0
- FdeChunks now used for both architectures (Tofino, Tofino2)
- FdeChunks are now unions of CLOT, PHV, CSUM
2.3.0
- Refactored ElementUsage to have only used_by and used_for, both required
- Created dedicated ElementUsageXbar and ElementUsageHash classes
- Changed Vliw element usage to pair used_by, action_names
- Changed Logical table element usage into table_name
- Changed action bus byte usages into array of used_by table names
2.4.0
- Changed used_for field from ElementUsage, ElementUsageXbar and HashBitUsage classes to be an enum
- Changed type of elements in usages field in HashDistributionUnitUsage from ElementUsage to new
  class ElementUsageHashDistribution
- Replaced usages array in Phase0ResourceUsage by fields used_by and used_for directly
  in Phase0ResourceUsage as compiler could produce at most 1 element of the usages array.
2.4.1
- allow color up to 3 (experimental)
2.4.2
- add information re CLOT extractions
"""

major_version = 2
medium_version = 4
minor_version = 2


def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(medium_version), str(minor_version))


"""
The array returned by this function needs to be kept consistent with the values
returned by function: std::string typeName(Memories::Use::type_t type)
in bf-p4c/logging/resources.cpp.
"""


def get_element_usage_used_for_enum_fields():
    return [
        "match_entry_ram",
        "algorithmic_tcam",
        "ternary_match",
        "gateway",
        "ternary_indirection_ram",
        "statistics_ram",
        "meter_ram",
        "meter_lpf",
        "meter_wred",
        "stateful_ram",
        "select_ram",
        "action_ram",
        "idletime",
    ]


"""
The array returned by this function needs to be kept consistent with the values
returned by following methods from bf-p4c/mau/input_xbar.cpp:
- std::string IXBar::Use::used_for()
- std::string IXBar::hash_dist_name(HashDistDest_t dest)
"""


def get_element_usage_xbar_used_for_enum_fields():
    return [
        "ternary_match",
        "atcam_match",
        "exact_match",
        "gateway",
        "selection",
        "meter",
        "stateful_alu",
        "immediate lo",
        "immediate hi",
        "stats address",
        "meter address",
        "action data address",
        "meter precolor",
        "selector mod",
    ]


########################################################


class ChunkType(jsl.StringField):
    title = "ChunkType"
    description = "Union for chunks"
    _FORMAT = 'chunk_type'


#### DEVICE RESOURCE USAGE ####


class ResourceUsage(jsl.Document):
    title = "ResourceUsage"
    description = "Resource usage context information."

    class ElementUsage(jsl.Document):
        title = "ElementUsage"
        description = "Resource usage for a particular resource."
        used_by = jsl.StringField(
            required=True, description="The P4 table name that is mapped to Phase 0."
        )
        used_for = jsl.StringField(
            required=True,
            enum=get_element_usage_used_for_enum_fields(),
            description="A short description of what this resource is being used for.",
        )

    class ElementUsageHashDistribution(jsl.Document):
        title = "ElementUsageHashDistribution"
        description = "Resource usage for a particular resource."
        used_by = jsl.StringField(
            required=True, description="The P4 table name that is mapped to Phase 0."
        )
        used_for = jsl.StringField(
            required=True,
            description="A short description of what this resource is being used for.",
        )

    class ElementUsageXbar(jsl.Document):
        title = "ElementUsageXbar"
        description = "Resource usage for particular input xbar byte."

        class XbarByteFieldSlice(jsl.Document):
            title = "XbarByteFieldSlice"
            description = "PHV field slice that is sourced by particular input xbar byte."
            field_name = jsl.StringField(
                required=True, description="Name of PHV field this slice belongs to."
            )
            msb = jsl.IntField(required=True, description="Most significant bit of the slice")
            lsb = jsl.IntField(required=True, description="Least significant bit of the slice")
            offset = jsl.IntField(
                required=True, description="Starting bit position within input xbar byte"
            )

        used_by = jsl.StringField(required=True, description="The P4 table name using this byte.")
        used_for = jsl.StringField(
            required=True,
            enum=get_element_usage_xbar_used_for_enum_fields(),
            description="What is this byte used for.",
        )
        slices = jsl.ArrayField(
            jsl.DocumentField("XbarByteFieldSlice", as_ref=True),
            required=True,
            min_items=1,
            description="What field slices are sourced by this particular usage.",
        )

    class ElementUsageHash(jsl.Document):
        title = "ElementUsageHash"
        description = "Resource usage for particular hash bit"
        type = jsl.StringField(
            required=True,
            enum=["way_select", "way_line_select", "selection_bit", "dist_bit", "gateway"],
            description="How is this bit used (what does value mean):"
            "  way_select      - Hash Way <value> RAM select (<value> is way index)"
            "  way_line_select - Hash Way <value> RAM line select (<value> is way index)"
            "  selection_bit   - Selection Hash Bit <value> (<value> is bit)"
            "  dist_bit        - Hash Dist Bit <value> (<value> is position)"
            "  gateway         - <field>[value] (<value> is bit index within a field)"
            " If type is 'gateway' then this bit is one of the top 12bits and it is "
            " used as a input to gateway logic.",
        )
        value = jsl.IntField(
            required=True, description="Usage value. Refer to type on what does this mean."
        )
        field_name = jsl.StringField(required=False, description="Only used for type == gateway")

    class ParserStateTransitionResourceUsage(jsl.Document):
        title = "ParserStateTransitionResourceUsage"
        description = "Resource usage per parser state transition."
        next_state_name = jsl.StringField(
            required=True, description="Name of the state in this row."
        )
        next_state_id = jsl.IntField(required=True, description="Unique state identifier.")
        tcam_row = jsl.IntField(
            required=True, enum=list(range(0, 256)), description="Parser TCAM row."
        )
        shifts = jsl.IntField(
            required=True,
            enum=list(range(0, 33)),
            description="Number of bytes shifted in the buffer.",
        )

        extracts = jsl.ArrayField(
            jsl.DictField(
                properties={
                    "extractor_id": jsl.IntField(
                        required=True, description="Which extractor is being used."
                    ),
                    "bit_width": jsl.IntField(required=True, description="Size of extractor."),
                    "buffer_offset": jsl.IntField(
                        required=False, description="Offset in the input buffer."
                    ),
                    "constant_value": jsl.IntField(
                        required=False, description="Constant to be extracted."
                    ),
                    "dest_container": jsl.IntField(
                        required=True, description="PHV container address."
                    ),
                }
            ),
            required=True,
            description="List of extractors in use.",
        )

        clot_extracts = jsl.ArrayField(
            jsl.DictField(
                properties={
                    "tag": jsl.IntField(required=True, description="CLOT tag"),
                    "buffer_offset": jsl.IntField(required=True, description="CLOT buffer offset"),
                    "length": jsl.IntField(required=True, description="CLOT length [in bytes]"),
                }
            ),
            required=False,
            description="List of CLOT extractions",
        )

        matchesOn = jsl.ArrayField(
            jsl.DictField(
                properties={
                    "hardware_id": jsl.IntField(
                        required=True, description="Register ID being matched on."
                    ),
                    "bit_width": jsl.IntField(required=True, description="Size of the register."),
                    "value_set": jsl.StringField(
                        required=False, description="Parser value set name matched on."
                    ),
                    "value": jsl.StringField(
                        required=False, description="Value matched on with mask."
                    ),
                }
            ),
            required=True,
            description="List of matches in use.",
        )
        savesTo = jsl.ArrayField(
            jsl.DictField(
                properties={
                    "hardware_id": jsl.IntField(
                        required=True, description="Register ID to save to."
                    ),
                    "buffer_offset": jsl.IntField(
                        required=True, description="Offset in the input buffer."
                    ),
                }
            ),
            required=False,
            description="List of saved match bytes/half words from the input buffer.",
        )
        has_counter = jsl.BooleanField(
            required=True, description="True if the state uses a counter."
        )
        previous_state_id = jsl.IntField(
            required=True,
            description="The id of the parser state that directly preceded entering this TCAM row.",
        )
        previous_state_name = jsl.StringField(
            required=False, description="Name of the previous state that leads to this TCAM row."
        )

    class ParserResourceUsage(jsl.Document):
        title = "ParserResourceUsage"
        description = "Resource usage context information for a parser."
        phase0 = jsl.DocumentField(
            "Phase0ResourceUsage", as_ref=True, description="Phase 0 resource utilization."
        )
        parser_id = jsl.IntField(required=True, enum=list(range(0, 18)), description="Parser ID")
        gress = jsl.StringField(
            required=True,
            enum=["ingress", "egress"],
            description="The gress this parser belongs to.",
        )
        nStates = jsl.IntField(
            required=True, description="Number of states available in the parser (TCAM rows)."
        )
        states = jsl.ArrayField(
            jsl.DocumentField("ParserStateTransitionResourceUsage", as_ref=True),
            required=True,
            description="Array of state resource utilization.",
        )

    class ParserResources(jsl.Document):
        title = "ParserResources"
        nParsers = jsl.IntField(required=True, description="Total number of parsers")
        parsers = jsl.ArrayField(
            jsl.DocumentField("ParserResourceUsage", as_ref=True),
            required=True,
            description="Array of parser resource utilization information.",
        )

    class Phase0ResourceUsage(jsl.Document):
        title = "Phase0ResourceUsage"
        description = "Resource usage context information for 'phase 0'."
        used_by = jsl.StringField(
            required=True, description="The P4 table name that is mapped to Phase 0."
        )
        used_for = jsl.StringField(
            required=True,
            description="A short description of what this resource is being used for.",
        )

    class MauResources(jsl.Document):
        title = "MauResources"
        nStages = jsl.IntField(required=True, description="Total number of MAU stages available.")
        mau_stages = jsl.ArrayField(
            jsl.DocumentField("MauStageResourceUsage", as_ref=True),
            required=True,
            description="Array of MAU stage resource utilization information.",
        )

    class MauStageResourceUsage(jsl.Document):
        title = "MauResourceUsage"
        description = "Resource usage context information for one MAU stage."
        stage_number = jsl.IntField(required=True, description="MAU stage number.")

        class XbarByteUsage(jsl.Document):
            title = "XbarByteUsage"
            description = "Resource usage for a match input crossbar byte."
            byte_number = jsl.IntField(
                required=True,
                enum=list(range(0, 194)),
                description="Match input crossbar byte number.",
            )
            byte_type = jsl.StringField(
                required=True,
                enum=["exact", "ternary"],
                description="Indicates if the match input crossbar byte is in the exact or ternary region.",
            )
            # match_group = jsl.IntField(description="The match input group this byte belongs to.")
            # parity_group = jsl.IntField(description="The match input parity group this byte belongs to.")
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsageXbar", as_ref=True),
                required=True,
                description="Array of how this match input crossbar byte is used.",
            )

        class XbarResourceUsage(jsl.Document):
            title = "XbarResourceUsage"
            description = "Resource usage for the match input crossbar"
            size = jsl.IntField(
                required=True, description="Total number of bytes in the match input crossbar."
            )
            exact_size = jsl.IntField(
                required=True,
                description="Total number of bytes in the exact match section of the match input crossbar.",
            )
            ternary_size = jsl.IntField(
                required=True,
                description="Total number of bytes in the ternary match section of the match input crossbar.",
            )
            bytes = jsl.ArrayField(
                jsl.DocumentField("XbarByteUsage", as_ref=True),
                required=True,
                description="Array of used match input crossbar byte resource utilization information."
                "Usages might be mutually exclusive if PHV headers are exclusive or same data "
                "might be used by multiple sources and/or multiple uses or byte contains "
                "non-related slices, placed in non-overlapping way so multiple usages can "
                "share physical space on the bus",
            )

        class HashBitUsage(jsl.Document):
            title = "HashBitUsage"
            description = "Resource usage for a hash bit."
            hash_bit = jsl.IntField(
                required=True, enum=list(range(0, 52)), description="Hash bit number."
            )
            hash_function = jsl.IntField(
                required=True, enum=list(range(0, 8)), description="Hash function number."
            )
            used_by_table = jsl.StringField(
                required=True, description="The P4 table name using this bit."
            )
            used_for = jsl.StringField(
                required=True,
                enum=get_element_usage_xbar_used_for_enum_fields(),
                description="What is this bit used for.",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsageHash", as_ref=True),
                required=True,
                description="Array of how this hash bit is used.",
            )

        class HashBitsResourceUsage(jsl.Document):
            title = "HashBitsResourceUsage"
            description = "Resource usage for the hash bits"
            nBits = jsl.IntField(
                required=True, description="Number of hash bits for a hash function."
            )
            nFunctions = jsl.IntField(required=True, description="Number of hash functions.")
            bits = jsl.ArrayField(
                jsl.DocumentField("HashBitUsage", as_ref=True),
                required=True,
                description="Array of used hash bits.",
            )

        class HashDistributionUnitUsage(jsl.Document):
            title = "HashDistributionUnitUsage"
            description = "Resource usage for a hash distribution unit."
            hash_id = jsl.IntField(
                required=True,
                enum=[0, 1],
                description="Hash function ID.  Hash distribution can connect to up to 2 of the 8 hash functions.",
            )
            unit_id = jsl.IntField(
                required=True,
                enum=[0, 1, 2],
                description="Hash distribution unit ID in this hash function.",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsageHashDistribution", as_ref=True),
                required=True,
                description="Array of how this hash distribution unit is used.",
            )

        class HashDistributionResourceUsage(jsl.Document):
            title = "HashDistributionResourceUsage"
            description = "Resource usage for the hash distribution units."
            nHashIds = jsl.IntField(
                required=True, description="Total number of hash ids in a hash distribution unit."
            )
            nUnitIds = jsl.IntField(
                required=True, description="Number of hash groups in a hash distribution unit."
            )
            units = jsl.ArrayField(
                jsl.DocumentField("HashDistributionUnitUsage", as_ref=True),
                required=True,
                description="Array of used hash distribution units.",
            )

        class RamUsage(jsl.Document):
            title = "RamUsage"
            description = "Resource usage for a RAM unit."
            row = jsl.IntField(
                required=True,
                enum=list(range(0, 8)),
                description="The physical row this RAM is on.",
            )
            way = jsl.IntField(
                required=False,
                enum=list(range(0, 80)),
                description="The way this RAM is on (for exact matches only).",
            )
            column = jsl.IntField(
                required=True,
                enum=list(range(0, 10)),
                description="The physical column this RAM is on.",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this RAM unit is used.",
            )

        class RamResourceUsage(jsl.Document):
            title = "RamResourceUsage"
            description = "Resource usage for the SRAM units."
            nRows = jsl.IntField(required=True, description="Total number of rows.")
            nColumns = jsl.IntField(required=True, description="Total number of columns.")
            srams = jsl.ArrayField(
                jsl.DocumentField("RamUsage", as_ref=True),
                required=True,
                description="Array of used rams.",
            )

        class MapRamUsage(jsl.Document):
            title = "MapRamUsage"
            description = "Resource usage for a Map RAM unit."
            row = jsl.IntField(
                required=True,
                enum=list(range(0, 8)),
                description="The physical row this Map RAM is on.",
            )
            unit_id = jsl.IntField(
                required=True,
                enum=list(range(0, 6)),
                description="The unit ID of this Map RAM.  Note that unit ID 0 corresponds to column 6, 1 to 7, etc.",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this Map RAM unit is used.",
            )

        class MapRamResourceUsage(jsl.Document):
            title = "MapRamResourceUsage"
            description = "Resource usage for the Map RAM units."
            nRows = jsl.IntField(required=True, description="Total number of rows.")
            nUnits = jsl.IntField(required=True, description="Total number of units per row.")
            maprams = jsl.ArrayField(
                jsl.DocumentField("MapRamUsage", as_ref=True),
                required=True,
                description="Array of used Map RAM units.",
            )

        class GatewayUsage(jsl.Document):
            title = "GatewayUsage"
            description = "Resource usage for a Gateway unit."
            row = jsl.IntField(
                required=True,
                enum=list(range(0, 8)),
                description="The physical row this Gateway is on.",
            )
            unit_id = jsl.IntField(
                required=True, enum=[0, 1], description="The unit ID of this Gateway."
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this Gateway unit is used.",
            )

        class GatewayResourceUsage(jsl.Document):
            title = "GatewayResourceUsage"
            description = "Resource usage for the Gateway units."
            nRows = jsl.IntField(required=True, description="Total number of rows.")
            nUnits = jsl.IntField(required=True, description="Total number of units per row.")
            gateways = jsl.ArrayField(
                jsl.DocumentField("GatewayUsage", as_ref=True),
                required=True,
                description="Array of used Gateways.",
            )

        class StashUsage(jsl.Document):
            title = "StashUsage"
            description = "Resource usage for a stash unit."
            row = jsl.IntField(
                required=True,
                enum=list(range(0, 8)),
                description="The physical row this Stash is on.",
            )
            unit_id = jsl.IntField(
                required=True, enum=[0, 1], description="The unit ID of this stash."
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this Stash unit is used.",
            )

        class StashResourceUsage(jsl.Document):
            title = "StashResourceUsage"
            description = "Resource usage for the Stash units."
            nRows = jsl.IntField(required=True, description="Total number of rows.")
            nUnits = jsl.IntField(required=True, description="Total number of units per row.")
            stashes = jsl.ArrayField(
                jsl.DocumentField("StashUsage", as_ref=True),
                required=True,
                description="Array of used Stashes.",
            )

        class MeterAluUsage(jsl.Document):
            title = "MeterAluUsage"
            description = "Resource usage for a Meter ALU."
            row = jsl.IntField(
                required=True,
                enum=[1, 3, 5, 7],
                description="The physical row this Meter ALU is on.",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this Meter ALU unit is used.",
            )

        class MeterAluResourceUsage(jsl.Document):
            title = "MeterAluResourceUsage"
            description = "Resource usage for the Meter ALU units."
            nAlus = jsl.IntField(required=True, description="Total number of ALUs.")
            meters = jsl.ArrayField(
                jsl.DocumentField("MeterAluUsage", as_ref=True),
                required=True,
                description="Array of used Meter ALUs.",
            )

        class StatisticAluUsage(jsl.Document):
            title = "StatisticAluUsage"
            description = "Resource usage for a Statistics ALU."
            row = jsl.IntField(
                required=True,
                enum=[0, 2, 4, 6],
                description="The physical row this Statistics ALU is on.",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this Statistics ALU unit is used.",
            )

        class StatisticAluResourceUsage(jsl.Document):
            title = "StatisticAluResourceUsage"
            description = "Resource usage for the Statistic ALU units."
            nAlus = jsl.IntField(required=True, description="Total number of ALUs.")
            stats = jsl.ArrayField(
                jsl.DocumentField("StatisticAluUsage", as_ref=True),
                required=True,
                description="Array of used Statistic ALUs.",
            )

        class TcamUsage(jsl.Document):
            title = "TcamUsage"
            description = "Resource usage for a TCAM unit."
            row = jsl.IntField(
                required=True,
                enum=list(range(0, 12)),
                description="The physical row this TCAM is on.",
            )
            column = jsl.IntField(
                required=True, enum=[0, 1], description="The physical column this TCAM is on."
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this TCAM unit is used.",
            )

        class TcamResourceUsage(jsl.Document):
            title = "TcamResourceUsage"
            description = "Resource usage for the TCAM units."
            nRows = jsl.IntField(required=True, description="Total number of rows.")
            nColumns = jsl.IntField(required=True, description="Total number of columns.")
            tcams = jsl.ArrayField(
                jsl.DocumentField("TcamUsage", as_ref=True),
                required=True,
                description="Array of used TCAMs.",
            )

        class ActionDataByteUsage(jsl.Document):
            title = "ActionDataByteUsage"
            description = "Resource usage for an action data bus byte."
            byte_number = jsl.IntField(
                required=True, enum=list(range(0, 128)), description="Action data bus byte number."
            )
            used_by_tables = jsl.ArrayField(
                jsl.StringField(), required=True, description="Names of P4 tables using this byte."
            )

        class ActionDataResourceUsage(jsl.Document):
            title = "ActionDataResourceUsage"
            description = "Resource usage for the action data bus."
            size = jsl.IntField(
                required=True, description="Total number of bytes in the action data bus."
            )
            bytes = jsl.ArrayField(
                jsl.DocumentField("ActionDataByteUsage", as_ref=True),
                required=True,
                description="Array of used action data bus bytes.",
            )

        class ActionSlotUsage(jsl.Document):
            title = "ActionSlotUsage"
            description = "Count of how many action slots of a particular size are used."
            slot_bit_width = jsl.IntField(
                required=True, enum=[8, 16, 32], description="Action data bus slot bit width."
            )
            maximum_slots = jsl.IntField(
                required=True, description="Maximum number of available slots of this size."
            )
            number_used = jsl.IntField(
                required=True, description="The number of slots of this size in use."
            )

        class VliwUsage(jsl.Document):
            title = "VliwUsage"
            description = "Resource usage for a VLIW instruction."
            instruction_number = jsl.IntField(
                required=True, enum=list(range(0, 32)), description="VLIW instruction number."
            )

            class VliwColorUsage(jsl.Document):
                title = "VliwColorUsage"
                description = "Resouce usage for a particular color and gress pair."

                color = jsl.IntField(
                    required=True, enum=[0, 1, 2, 3], description="VLIW instruction color."
                )
                gress = jsl.StringField(
                    required=True,
                    enum=["ingress", "egress", "ghost"],
                    description="The gress this VLIW instruction belongs to.",
                )
                usages = jsl.ArrayField(
                    jsl.DictField(
                        properties={
                            "used_by": jsl.StringField(
                                required=True, description="Name of the P4 table."
                            ),
                            "action_names": jsl.ArrayField(
                                jsl.StringField(),
                                min_items=1,
                                required=True,
                                description="List of actions using this VLIW instruction.",
                            ),
                        }
                    )
                )

            color_usages = jsl.ArrayField(
                jsl.DocumentField("VliwColorUsage", as_ref=True),
                required=True,
                description="Array of color/gress pairs for this VLIW instruction.",
            )

        class VliwResourceUsage(jsl.Document):
            title = "VliwResourceUsage"
            description = "Resource usage for the VLIW instructions."
            size = jsl.IntField(required=True, description="Total number of instructions.")
            instructions = jsl.ArrayField(
                jsl.DocumentField("VliwUsage", as_ref=True),
                required=True,
                description="Array of used VLIW instructions.",
            )

        class LogicalTableUsage(jsl.Document):
            title = "LogicalTableUsage"
            description = "Resource usage for an action data bus byte."
            id = jsl.IntField(
                required=True, enum=list(range(0, 16)), description="The logical table ID."
            )
            table_name = jsl.StringField(required=True, description="Name of the P4 table.")

        class LogicalTableResourceUsage(jsl.Document):
            title = "LogicalTableResourceUsage"
            description = "Resource usage for the logical table IDs."
            size = jsl.IntField(required=True, description="Total number of ids.")
            ids = jsl.ArrayField(
                jsl.DocumentField("LogicalTableUsage", as_ref=True),
                required=True,
                description="Array of used logical table IDs.",
            )

        class ExactMatchSearchBusUsage(jsl.Document):
            title = "ExactMatchSearchBusUsage"
            description = "Resource usage for an exact match search bus."
            id = jsl.IntField(
                required=True,
                enum=list(range(0, 16)),
                description="The search bus ID.  There are two search buses per SRAM row.  The SRAM row is floor(id/2).",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this exact match search bus is used.",
            )

        class ExactMatchSearchBusResourceUsage(jsl.Document):
            title = "ExactMatchSearchBusResourceUsage"
            description = "Resource usage for the exact match search buses."
            size = jsl.IntField(
                required=True, description="Total number of exact match search buses."
            )
            ids = jsl.ArrayField(
                jsl.DocumentField("ExactMatchSearchBusUsage", as_ref=True),
                required=True,
                description="Array of used exact match search buses.",
            )

        class ExactMatchResultBusUsage(jsl.Document):
            title = "ExactMatchResultBusUsage"
            description = "Resource usage for an exact match result bus."
            id = jsl.IntField(
                required=True,
                enum=list(range(0, 16)),
                description="The result bus ID.  There are two result buses per SRAM row.  The SRAM row is floor(id/2).",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this exact match result bus is used.",
            )

        class ExactMatchResultBusResourceUsage(jsl.Document):
            title = "ExactMatchResultBusResourceUsage"
            description = "Resource usage for the exact match result buses."
            size = jsl.IntField(
                required=True, description="Total number of exact match result buses."
            )
            ids = jsl.ArrayField(
                jsl.DocumentField("ExactMatchResultBusUsage", as_ref=True),
                required=True,
                description="Array of used exact match result buses.",
            )

        class TindResultBusUsage(jsl.Document):
            title = "TindResultBusUsage"
            description = "Resource usage for a ternary indirect result bus."
            id = jsl.IntField(
                required=True,
                enum=list(range(0, 16)),
                description="The result bus ID.  There are two result buses per SRAM row.  The SRAM row is floor(id/2).",
            )
            usages = jsl.ArrayField(
                jsl.DocumentField("ElementUsage", as_ref=True),
                required=True,
                description="Array of how this ternary indirect result bus is used.",
            )

        class TindResultBusResourceUsage(jsl.Document):
            title = "TindResultBusResourceUsage"
            description = "Resource usage for the ternary indirect result buses."
            size = jsl.IntField(
                required=True, description="Total number of ternary indirect result buses."
            )
            ids = jsl.ArrayField(
                jsl.DocumentField("TindResultBusUsage", as_ref=True),
                required=True,
                description="Array of used ternary indirect result buses.",
            )

        xbar_bytes = jsl.DocumentField(
            "XbarResourceUsage",
            as_ref=True,
            required=True,
            description="The match input crossbar resource utilization information.",
        )
        hash_bits = jsl.DocumentField(
            "HashBitsResourceUsage",
            as_ref=True,
            required=True,
            description="Hash bits resource utilization information.",
        )
        hash_distribution_units = jsl.DocumentField(
            "HashDistributionResourceUsage",
            as_ref=True,
            required=True,
            description="Hash distribution unit resource utilization information.",
        )
        rams = jsl.DocumentField(
            "RamResourceUsage",
            as_ref=True,
            required=True,
            description="RAM unit resource utilization information.",
        )
        map_rams = jsl.DocumentField(
            "MapRamResourceUsage",
            as_ref=True,
            required=True,
            description="Map RAM resource utilization information.",
        )
        gateways = jsl.DocumentField(
            "GatewayResourceUsage",
            as_ref=True,
            required=True,
            description="Gateway resource utilization information.",
        )
        stashes = jsl.DocumentField(
            "StashResourceUsage",
            as_ref=True,
            required=True,
            description="Stash resource utilization information.",
        )
        meter_alus = jsl.DocumentField(
            "MeterAluResourceUsage",
            as_ref=True,
            required=True,
            description="Meter ALU resource utilization information.",
        )
        statistic_alus = jsl.DocumentField(
            "StatisticAluResourceUsage",
            as_ref=True,
            required=True,
            description="Statistics ALU resource utilization information.",
        )
        tcams = jsl.DocumentField(
            "TcamResourceUsage",
            as_ref=True,
            required=True,
            description="TCAM unit resource utilization information.",
        )
        action_bus_bytes = jsl.DocumentField(
            "ActionDataResourceUsage",
            as_ref=True,
            required=True,
            description="Action data bus byte resource utilization information.",
        )
        action_slots = jsl.ArrayField(
            jsl.DocumentField("ActionSlotUsage", as_ref=True),
            required=True,
            description="Array of action data bus slot utilization information.",
        )
        vliw = jsl.DocumentField(
            "VliwResourceUsage",
            as_ref=True,
            required=True,
            description="VLIW instruction resource utilization information.",
        )
        logical_tables = jsl.DocumentField(
            "LogicalTableResourceUsage",
            as_ref=True,
            required=True,
            description="Logical table resource utilization information.",
        )
        exm_search_buses = jsl.DocumentField(
            "ExactMatchSearchBusResourceUsage",
            as_ref=True,
            required=True,
            description="Exact match search bus resource utilization information.",
        )
        exm_result_buses = jsl.DocumentField(
            "ExactMatchResultBusResourceUsage",
            as_ref=True,
            required=True,
            description="Exact match result bus resource utilization information.",
        )
        tind_result_buses = jsl.DocumentField(
            "TindResultBusResourceUsage",
            as_ref=True,
            required=True,
            description="Ternary indirect result bus resource utilization information.",
        )

    class DeparserResourceUsage(jsl.Document):
        title = "DeparserResourceUsage"
        description = "Resource usage context information for a deparser."
        gress = jsl.StringField(
            required=True,
            enum=["ingress", "egress"],
            description="The gress this deparser belongs to.",
        )

        class Pov(jsl.Document):
            title = "Pov"
            description = "Information about the packet occupancy vector (POV)."

            size = jsl.IntField(required=True, description="The number of POV bits available.")

            pov_bits = jsl.ArrayField(
                items=jsl.DictField(
                    properties={
                        "pov_bit": jsl.IntField(
                            required=True,
                            description="The index this bit is associated with in the packet occupancy vector.",
                        ),
                        "phv_container": jsl.IntField(
                            required=False,
                            description="The PHV container this POV bit is sourced from.",
                        ),
                        "phv_container_bit": jsl.IntField(
                            required=True,
                            description="The bit in the PHV container that holds this POV bit, where 0 is the least significant bit of a container.",
                        ),
                        "pov_name": jsl.StringField(
                            required=False,
                            description="The compiler given name for this POV bit, e.g. ethernet.",
                        ),
                    }
                ),
                required=True,
                description="An array of POV bits in use.",
            )

        class FdeUsage(jsl.Document):
            title = "FdeUsage"
            description = "Information about usage of a specific field dictionary entry."
            entry = jsl.IntField(required=True, description="The field dictionary entry number.")
            pov_bit = jsl.IntField(
                required=True, description="The POV bit index that enables this entry."
            )
            pov_name = jsl.StringField(
                required=True, description="The compiler given name for the POV bit, e.g. ethernet."
            )

            chunks = jsl.ArrayField(
                items=jsl.DocumentField("FdeChunk", as_ref=True),
                required=False,
                description="An array of deparsed chunk usage information.",
            )

        class ChunkPhv(jsl.Document):
            title = "ChunkPhv"
            description = "A PHV chunk"

            chunk_type = ChunkType("chunk_phv")
            phv_container = jsl.IntField(
                required=True, description="The PHV container address to pull a byte from."
            )

        class ChunkClot(jsl.Document):
            title = "ChunkClot"
            description = "A CLOT chunk"

            chunk_type = ChunkType("chunk_clot")
            clot_tag = jsl.IntField(required=True, description="The CLOT tag to pull from.")

        class ChunkCsum(jsl.Document):
            title = "ChunkCsumt"
            description = "A CSUM chunk"

            chunk_type = ChunkType("chunk_csum")
            csum_engine = jsl.IntField(
                required=True, description="The CSUM engine number to pull from."
            )

        class FdeChunk(jsl.Document):
            title = "FdeChunk"
            description = "A chunk from a deparser field dictionary entry."

            chunk_number = jsl.IntField(
                required=True,
                enum=list(range(0, 127)),
                description="The chunk number. The most significant chunk (i.e. the first to be deparsed) will have lower index.",
            )
            chunk = jsl.OneOfField(
                [
                    jsl.DocumentField('ChunkPhv', as_ref=True),
                    jsl.DocumentField('ChunkClot', as_ref=True),
                    jsl.DocumentField('ChunkCsum', as_ref=True),
                ],
                required=True,
            )

        class DeparserTable(jsl.Document):
            title = "DeparserTable"
            description = "Information about a specific deparser table usage."

            nTables = jsl.IntField(
                required=True,
                description="The number of unique tables supported for this deparser table type.",
            )
            maxBytes = jsl.IntField(
                required=True,
                description="The maximum number of bytes that this table type can support.",
            )
            index_phv = jsl.IntField(
                required=False,
                description="The PHV container address that holds the table index select.  Not required if this table type is not used.",
            )
            table_phv = jsl.ArrayField(
                items=jsl.DictField(
                    properties={
                        "table_id": jsl.IntField(
                            required=True,
                            description="The table index.  Allowed values are [0:nTables).",
                        ),
                        "field_list_name": jsl.StringField(
                            required=False,
                            description="The name of the field list this table is allocated for.",
                        ),
                        "bytes": jsl.ArrayField(
                            items=jsl.DictField(
                                properties={
                                    "byte_number": jsl.IntField(
                                        required=True,
                                        description="The byte number in the table.  The most significant byte (i.e. the first to be output) will have lower indicies.",
                                    ),
                                    "phv_container": jsl.IntField(
                                        required=True,
                                        description="The PHV container address to pull a byte from.",
                                    ),
                                }
                            ),
                            required=True,
                            description="An array indicating where each byte in the table is sourced from.",
                        ),
                    }
                ),
                required=True,
                description="An array of the table information that can be deparsed.",
            )

        pov = jsl.DocumentField(
            "Pov",
            as_ref=True,
            required=True,
            description="Information about the packet occupancy vector (POV).",
        )

        nFdeEntries = jsl.IntField(
            required=True, description="The number of field dictionary entries available."
        )
        fde_entries = jsl.ArrayField(
            items=jsl.DocumentField("FdeUsage", as_ref=True),
            required=True,
            description="Array of field dictionary entry usage information.",
        )

        # FIXME: These also changed in Tofino2?
        mirror_table = jsl.DocumentField(
            "DeparserTable",
            as_ref=True,
            required=True,
            description="Information about usage of the mirror table.",
        )
        # On Tofino, these are only available in the ingress deparser.
        resubmit_table = jsl.DocumentField(
            "DeparserTable",
            as_ref=True,
            required=False,
            description="Information about usage of the resubmit table.",
        )
        learning_table = jsl.DocumentField(
            "DeparserTable",
            as_ref=True,
            required=False,
            description="Information about usage of the learning table (digests).",
        )

    class PhvResourceUsage(jsl.Document):
        title = "PhvResourceUsage"
        description = "PHV container resources available."

        class PhvContainerType(jsl.Document):
            title = "PhvContainerType"
            description = "Information about specific PHV container types."

            width = jsl.IntField(required=True, description="The bit width of each container.")
            units = jsl.IntField(
                required=True, description="The number of containers of this type."
            )
            addresses = jsl.ArrayField(
                items=jsl.IntField(),
                required=True,
                description="Array of the container addresses available.",
            )

        normal = jsl.ArrayField(
            items=jsl.DocumentField("PhvContainerType", as_ref=True),
            required=True,
            description="Fully functional containers in the MAU space.",
        )
        tagalong = jsl.ArrayField(
            items=jsl.DocumentField("PhvContainerType", as_ref=True),
            required=False,
            description="Containers in tagalong space.",
        )
        mocha = jsl.ArrayField(
            items=jsl.DocumentField("PhvContainerType", as_ref=True),
            required=False,
            description="Limited function containers in the MAU space.",
        )
        dark = jsl.ArrayField(
            items=jsl.DocumentField("PhvContainerType", as_ref=True),
            required=False,
            description="Limited function containers in the MAU space.",
        )

    class ClotUsage(jsl.Document):
        title = "ClotUsage"
        description = "Resource usage per CLOT."

        field_lists = jsl.ArrayField(
            items=jsl.DocumentField("ClotField", as_ref=True),
            required=True,
            description="List of fields.",
        )

        issue_states = jsl.ArrayField(
            items=jsl.StringField(
                required=True, description="Name of the state the CLOT is issued (initiated)."
            ),
            required=True,
            min_items=1,
        )
        offset = jsl.IntField(
            required=True,
            enum=list(range(0, 32)),
            description="Byte offset of the CLOT relative to the beginning of the input buffer of the issue state.",
        )
        has_checksum = jsl.BooleanField(required=True, description="True if the CLOT has checksum.")
        tag = jsl.IntField(
            required=True, enum=list(range(0, 64)), description="Unique identifier of the CLOT."
        )
        length = jsl.IntField(
            required=True, enum=list(range(1, 65)), description="Length of the CLOT in byte."
        )

    class ClotResourceUsage(jsl.Document):
        title = "ClotResourceUsage"
        description = "CLOT (Tuple of Checksum, Length, Offset, Tag) resources available."

        gress = jsl.StringField(
            required=True, enum=["ingress", "egress"], description="The gress this CLOT belongs to."
        )
        num_clots = jsl.IntField(
            required=True, description="Number of unique CLOTs the parser HW can issue."
        )
        clots = jsl.ArrayField(
            items=jsl.DocumentField("ClotUsage", as_ref=True),
            required=True,
            description="Array of CLOT resource utilization.",
        )
        clot_eligible_fields = jsl.ArrayField(
            items=jsl.DocumentField("ClotEligibleField", as_ref=True),
            required=True,
            description="Allocation information for CLOT-eligible fields",
        )

    class ClotField(jsl.Document):
        title = "ClotField"
        description = "CLOT field information"
        name = jsl.StringField(required=True, description="Name of the field.")
        field_msb = jsl.IntField(
            required=True, description="Most significant bit of the field in this CLOT"
        )
        field_lsb = jsl.IntField(
            required=True, description="Least significant bit of the field in this CLOT"
        )
        clot_offset = jsl.IntField(
            required=True,
            description="The bit position of this field in the CLOT. CLOT[clot_offset:clot_offset+field_msb-field_lsb] = name[field_msb:field_lsb]",
        )

    class ClotEligibleField(jsl.Document):
        title = "ClotEligibleField"
        description = "CLOT-eligible field-allocation information"
        name = jsl.StringField(required=True, description="Name of the field.")
        is_readonly = jsl.BooleanField(
            required=True, description="True if the field is only read by MAU."
        )
        is_modified = jsl.BooleanField(
            required=True, description="True if the field is modified by MAU."
        )
        is_checksum = jsl.BooleanField(
            required=True, description="True if the field is a checksum."
        )
        bit_width = jsl.IntField(required=True, description="Number of bits in the field.")
        num_bits_in_clots = jsl.IntField(
            required=True, description="Number of bits allocated in CLOTs."
        )
        num_bits_in_phvs = jsl.IntField(
            required=True, description="Number of bits allocated in PHVs."
        )

    parser = jsl.DocumentField(
        "ParserResources",
        as_ref=True,
        required=True,
        description="Parser resource utilization information.",
    )
    mau = jsl.DocumentField(
        "MauResources",
        as_ref=True,
        required=True,
        description="MAU resource utilization information.",
    )
    deparser = jsl.ArrayField(
        jsl.DocumentField("DeparserResourceUsage", as_ref=True),
        required=True,
        description="Array of deparser resource utilization information.",
    )
    phv_containers = jsl.DocumentField(
        "PhvResourceUsage", as_ref=True, required=True, description="PHV resources available."
    )
    clots = jsl.ArrayField(
        items=jsl.DocumentField(
            "ClotResourceUsage",
            as_ref=True,
            required=False,
            description="CLOT resources available.",
        ),
        required=False,
        description="Array of CLOT resources per gress",
    )


# ------------------------------
#  Top level JSON schema
# ------------------------------


class ResourcesJSONSchema(jsl.Document):
    title = "PhvJSONSchema"
    description = "PHV JSON schema for a P4 program's PHV allocation results.."
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    build_date = jsl.StringField(
        required=True, description="Timestamp of when the program was built."
    )
    compiler_version = jsl.StringField(
        required=True, description="Compiler version used in compilation."
    )
    schema_version = jsl.StringField(
        required=True, description="Schema version used to produce this JSON."
    )

    resources = jsl.DocumentField(
        "ResourceUsage", as_ref=True, description="Device resource utilization."
    )

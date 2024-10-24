#!/usr/bin/env python3

"""
power_schema.py: Generates a JSON Schema model for structured power estimation information.
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
- Support up to 8 pipelines
"""

major_version = 1
medium_version = 0
minor_version = 1


def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(medium_version), str(minor_version))


########################################################

max_pipe_count = 8


class StageCharacteristics(jsl.Document):
    title = "StageCharacteristics"
    description = "Information about features enabled, latency, dependencies and other stage-specific characteristics."

    class Features(jsl.Document):
        title = "Features"
        description = "Information about what features are enabled."

        exact = jsl.BooleanField(
            required=True, description="A Boolean indicating if exact match resources are used."
        )
        ternary = jsl.BooleanField(
            required=True, description="A Boolean indicating if ternary resources are used."
        )
        statistics = jsl.BooleanField(
            required=True, description="A Boolean indicating if statistics (counters) are used."
        )
        lpf_wred = jsl.BooleanField(
            required=True,
            description="A Boolean indicating if LPF meters or WRED meters are in use.",
        )
        selectors = jsl.BooleanField(
            required=True, description="A Boolean indicating if selectors are in use."
        )
        max_selector_words = jsl.IntField(
            required=False,
            description="An integer indicating the maximum selector RAM words allocated for a given group.",
        )
        stateful = jsl.BooleanField(
            required=True, description="A Boolean indicating if stateful ALUs are in use."
        )

    stage_number = jsl.IntField(required=True, description="The stage number.")
    gress = jsl.StringField(
        required=True,
        enum=["ingress", "egress", "ghost"],
        description="The gress these characterisitcs apply to.",
    )

    dependency_to_previous = jsl.StringField(
        required=True,
        enum=["match", "action", "concurrent"],
        description="The dependency type this stage has on the previous stage.",
    )

    clock_cycles = jsl.IntField(
        required=True,
        desciption="The number of clock cycles this stage requires to complete all processing.",
    )
    predication_cycle = jsl.IntField(
        required=True, desciption="The clock cycle in which predication occurs."
    )
    cycles_contribute_to_latency = jsl.IntField(
        required=True,
        desciption="The number of clock cycles this stage contributes to the overall pipeline gress latency.",
    )

    features = jsl.DocumentField(
        "Features",
        as_ref=True,
        required=True,
        description="What MAU-specific features are used in each stage.",
    )
    features_to_balance_latency = jsl.DocumentField(
        "Features",
        as_ref=True,
        required=True,
        description="In an action/concurrent dependency chain, MAU features "
        "have to be 'enabled' to achieve latency balance.  "
        "These properties indicate a feature is not needed in "
        "a stage, but is 'turned on' from a latency perspecitive.",
    )


class MatchTables(jsl.Document):

    class StageDetails(jsl.Document):
        title = "StageDetails"
        description = "Information about table power usage on a per-stage basis."

        stage_number = jsl.IntField(
            required=True, description="The MAU stage where this section of the table resides."
        )
        weight = jsl.NumberField(
            required=True,
            description="The relative weighting that this table contributes to the total worst-case power.",
        )
        on_critical_path = jsl.BooleanField(
            required=True,
            description="A Boolean indicating if the table in this stage is on the critical control flow path.",
        )
        always_runs = jsl.BooleanField(
            required=True,
            description="A Boolean indicating if the table in this stage, though not on the critical control flow path, will still contribute to worst case power.",
        )

        memories = jsl.ArrayField(
            items=jsl.DictField(
                properties={
                    "memory_type": jsl.StringField(
                        required=True,
                        enum=["sram", "tcam", "map_ram", "deferred_ram"],
                        description="The type of memory being used.",
                    ),
                    "access_type": jsl.StringField(
                        required=True,
                        enum=["read", "search", "write"],
                        description="The type of access performed on the memory.",
                    ),
                    "num_memories": jsl.IntField(
                        required=True, description="The number of memory blocks that are accessed."
                    ),
                },
                required=True,
                description="",
                additional_properties=False,
            ),
            required=True,
            description="A list of how this table uses memory in this stage, including any attached tables it may have.",
        )

    title = "Tables"
    description = "Table information."

    name = jsl.StringField(required=True, description="The name of the table.")
    gress = jsl.StringField(
        required=True,
        enum=["ingress", "egress", "ghost"],
        description="The gress the table belongs to.",
    )
    stages = jsl.ArrayField(
        items=jsl.DocumentField("StageDetails", as_ref=True, description=""),
        required=True,
        description="An array of the per-stage power information for this match table.",
    )


# ------------------------------
#  Top level JSON schema
# ------------------------------


class PowerJSONSchema(jsl.Document):
    title = "PowerJSONSchema"
    description = "Power JSON schema for a P4 program's table placement results."
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    build_date = jsl.StringField(
        required=True, description="Timestamp of when the program was built."
    )
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    compiler_version = jsl.StringField(
        required=True, description="Compiler version used in compilation."
    )
    schema_version = jsl.StringField(
        required=True, description="Schema version used to produce this JSON."
    )

    tables = jsl.ArrayField(
        items=jsl.DocumentField("MatchTables", as_ref=True),
        required=True,
        description="A list of match table allocation information.",
    )

    total_power = jsl.ArrayField(
        items=jsl.DictField(
            properties={
                "gress": jsl.StringField(
                    required=True,
                    enum=["ingress", "egress", "ghost"],
                    description="The gress this power number applies to.",
                ),
                "power": jsl.NumberField(
                    required=True, description="The total power consumed, in Watts, for this gress."
                ),
                "pipe_number": jsl.IntField(
                    required=True,
                    enum=range(0, max_pipe_count),
                    description="The logical pipe number the power was computed for.",
                ),
            },
            required=True,
            description="",
            additional_properties=False,
        ),
        required=True,
        description="A list of the power consumption per gress per pipe.",
    )

    stage_characteristics = jsl.ArrayField(
        items=jsl.DocumentField("StageCharacteristics", as_ref=True),
        required=True,
        description="An array of stage characteristics.",
    )

    total_latency = jsl.ArrayField(
        items=jsl.DictField(
            properties={
                "gress": jsl.StringField(
                    required=True,
                    enum=["ingress", "egress", "ghost"],
                    description="The gress this latency number applies to.",
                ),
                "latency": jsl.IntField(
                    required=True, description="The total latency, in cycles, for this pipeline."
                ),
                "pipe_number": jsl.IntField(
                    required=True,
                    enum=range(0, max_pipe_count),
                    description="The logical pipe number the latency was computed for.",
                ),
            },
            required=True,
            description="",
            additional_properties=False,
        ),
        required=True,
        description="A list of the total latency results per gress per pipe.",
    )

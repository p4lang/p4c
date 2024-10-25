#!/usr/bin/env python3

"""
table_graph_schema.py: Generates a JSON schema for table graph data
"""

import json

import jsl
from schemas.jgf_schema import Graph, GraphEdge, GraphNode, JsonGraphFormatSchema

########################################################
#   Schema Version
########################################################

"""
1.0.1
"""

major_version = 1  # (major)
revision_version = 0  # (medium)
addition_version = 1  # (minor)


def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(revision_version), str(addition_version))


########################################################


class TableGraphsSchema(JsonGraphFormatSchema):
    title = "TableGraphsSchema"
    description = "Table graph data as extended JGF (http://jsongraphformat.info/)"

    schema_version = jsl.StringField(
        required=True,
        enum=[get_schema_version()],
        description="Schema version used to produce this JSON.",
    )
    graphs = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("TableGraph", as_ref=True, required=True),
        description="An array of table graph data.",
    )


class TableGraph(Graph):
    title = "TableGraph"
    description = "Table graph data."

    nodes = jsl.ArrayField(
        jsl.DocumentField("TableGraphNode", as_ref=True),
        required=True,
        description="An array containing all graph nodes",
    )
    edges = jsl.ArrayField(
        jsl.DocumentField("TableGraphEdge", as_ref=True),
        required=True,
        description="An array containing all graph connections",
    )
    metadata = jsl.DictField(
        properties={
            "gress": jsl.StringField(
                required=True,
                enum=["ingress", "egress", "ghost"],
                description="Graph for ingress or egress pipeline.",
            ),
            "placement_complete": jsl.BooleanField(
                required=True, description="A flag indicating whether tables have been placed."
            ),
            "compile_iteration": jsl.StringField(
                required=False, description="The name of this compilation pass."
            ),
            "description": jsl.StringField(
                required=True, description="A description for this graph."
            ),
        },
        required=True,
        description="Table graph metadata.",
        additional_properties=False,
    )


# Graph Nodes


class TableGraphNode(GraphNode):
    title = "TableGraphNode"
    metadata = jsl.DocumentField(
        "NodeMetadata", as_ref=True, required=True, description="Table graph node metadata."
    )


class NodeMetadata(jsl.Document):
    title = "NodeMetadata"
    description = "Table metadata for this graph node."
    additional_properties = False

    tables = jsl.ArrayField(
        required=True,
        items=jsl.OneOfField(
            [
                jsl.DocumentField("AttachedTable", as_ref=True),
                jsl.DocumentField("MatchTable", as_ref=True),
                jsl.DocumentField("ConditionTable", as_ref=True),
            ]
        ),
        description="A list of p4 table metadata at this graph node.",
    )
    placement = jsl.DictField(
        required=False,
        properties={
            "logical_table_id": jsl.NumberField(
                required=True,
                description="Unique identifier for this logical table within this stage.",
            ),
            "stage_number": jsl.NumberField(
                required=True,
                description="Number identifying the stage containing this stage table.",
            ),
        },
        description="Placement information for this graph node (can be undefined if graph.placement_complete is False).",
        additional_properties=False,
    )
    min_stage = jsl.IntField(
        required=True, description="The minimum stage this table can be placed."
    )
    dep_chain = jsl.IntField(
        required=True, description="The number of nodes following this node in the program flow."
    )


# Table metadata should be the minimum required data to lookup tables in other files like context.json
# without clashes with tables using the same table name.
class AttachedTable(jsl.Document):
    title = "AttachedTable"
    description = "Table metadata for tables that can be attached to match tables."
    additional_properties = False

    name = jsl.StringField(
        required=True,
        description="Table name (format must match other JSON artifacts like context.json).",
    )
    table_type = jsl.StringField(
        required=True,
        enum=[
            "action",
            "meter",
            "selection",
            "stateful",
            "statistics",
            "ternary_indirect",
            "idletime",
        ],
        description="Discriminator tag for this table type.",
    )


class MatchTable(jsl.Document):
    title = "MatchTable"
    description = "Match table metadata."
    additional_properties = False

    name = jsl.StringField(
        required=True,
        description="Table name (format must match other JSON artifacts like context.json).",
    )
    table_type = jsl.StringField(
        required=True, enum=["match"], description="Discriminator tag for this table type."
    )
    match_type = jsl.StringField(
        required=True,
        enum=[
            "exact",
            "ternary",
            "proxy_hash",
            "match_with_no_key",
            "hash_action",
            "algorithmic_lpm",
            "algorithmic_tcam",
            "chained_lpm",
        ],
        description="Match table match type.",
    )


class ConditionTable(jsl.Document):
    title = "ConditionTable"
    description = "Condition table metadata."

    name = jsl.StringField(
        required=True,
        description="Table name (format must match other JSON artifacts like context.json).",
    )
    table_type = jsl.StringField(
        required=True, enum=["condition"], description="Discriminator tag for this table type."
    )
    condition = jsl.StringField(
        required=True, description="Condition string of this condition table."
    )


# Graph Edges


class TableGraphEdge(GraphEdge):
    title = "TableGraphEdge"

    metadata = jsl.OneOfField(
        [
            jsl.DocumentField("EdgeMetadataDefaultNextTable", as_ref=True),
            jsl.DocumentField("EdgeMetadataControlExit", as_ref=True),
            jsl.DocumentField("EdgeMetadataCondition", as_ref=True),
            jsl.DocumentField("EdgeMetadataAction", as_ref=True),
            jsl.DocumentField("EdgeMetadataTableHitMiss", as_ref=True),
            jsl.DocumentField("EdgeMetadataActionRead", as_ref=True),
            jsl.DocumentField("EdgeMetadataAnti", as_ref=True),
            jsl.DocumentField("EdgeMetadataIxbarRead", as_ref=True),
            jsl.DocumentField("EdgeMetadataOutput", as_ref=True),
            jsl.DocumentField("EdgeMetadataContainerConflict", as_ref=True),
            jsl.DocumentField("EdgeMetadataExit", as_ref=True),
            jsl.DocumentField("EdgeMetadataReductionOr", as_ref=True),
        ],
        required=True,
        description="Table graph edge metadata.",
    )


class EdgeMetadataBase(jsl.Document):
    title = "EdgeMetadataBase"
    description = "Common definitions for edge metadata"
    additional_properties = False

    is_critical = jsl.BooleanField(
        required=False, description="A flag indicating if edge lies on a critical path."
    )
    tags = jsl.ArrayField(
        required=False,
        items=jsl.StringField(),
        description="An optional array of tags which can be used for grouping and filtering.",
    )


# Control Type Edges
class EdgeMetadataControlBase(EdgeMetadataBase):
    title = "EdgeMetadataControlBase"
    description = "Common definitions for control type edge metadata"
    additional_properties = False

    type = jsl.StringField(required=True, enum=["control"], description="Type of dependency.")


class EdgeMetadataDefaultNextTable(EdgeMetadataControlBase):
    title = "EdgeMetadataDefaultNextTable"
    description = "Default next-table pointer from a match table."

    sub_type = jsl.StringField(
        required=True, enum=["default_next_table"], description="Type of control edge."
    )


class EdgeMetadataControlExit(EdgeMetadataControlBase):
    title = "EdgeMetadataControlExit"
    description = "Edge to tables needing to run last, before exit."

    sub_type = jsl.StringField(
        required=True, enum=["control_exit"], description="Type of control edge."
    )


class EdgeMetadataCondition(EdgeMetadataControlBase):
    title = "EdgeMetadataCondition"
    description = "Condition next-table pointer."

    sub_type = jsl.StringField(
        required=True, enum=["condition"], description="Type of control edge."
    )
    condition_value = jsl.BooleanField(required=True, description="Value of conditional.")


class EdgeMetadataAction(EdgeMetadataControlBase):
    title = "EdgeMetadataAction"
    description = "Action next-table pointer from a match table."

    sub_type = jsl.StringField(required=True, enum=["action"], description="Type of control edge.")
    action_name = jsl.StringField(required=True, description="The name of the action.")


class EdgeMetadataTableHitMiss(EdgeMetadataControlBase):
    title = "EdgeMetadataTableHit"
    description = "Table hit or miss."

    sub_type = jsl.StringField(
        required=True, enum=["table_hit", "table_miss"], description="Type of control edge."
    )


class EdgeMetadataAnti(EdgeMetadataControlBase):
    title = "EdgeMetadataAnti"
    description = "Write after read data dependency."

    sub_type = jsl.StringField(
        required=True, enum=["anti"], description="Type of control dependency."
    )
    anti_type = jsl.StringField(
        required=True,
        enum=[
            "table_read",
            "action_read",
            "next_table_data",
            "next_table_control",
            "table_metadata",
        ],
        description="""
Type of anti data dependency.
  `table_read`: Action write after table read.
  `action_read`: Action write after action read.
  `next_table_data`: Data dependency between tables in separate blocks.
  `next_table_control`: Injected control dependency due to next table control flow.
  `next_table_metadata`: Data dependency between tables in separate blocks due to metadata fields.
""",
    )
    dep_fields = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("FieldSlice", as_ref=True),
        description="Dependency fields.",
    )


class EdgeMetadataExit(EdgeMetadataControlBase):
    title = "EdgeMetadataExit"
    description = "Action exit enforcing table execution order."

    sub_type = jsl.StringField(required=True, enum=["exit"], description="Type of control edge.")
    action_name = jsl.StringField(
        required=True, description="The name of the action which has the exit command."
    )


# Match Type Edges
class EdgeMetadataMatchBase(EdgeMetadataBase):
    title = "EdgeMetadataMatchBase"
    description = "Common definitions for match type edge metadata"

    type = jsl.StringField(required=True, enum=["match"], description="Type of dependency.")


class EdgeMetadataIxbarRead(EdgeMetadataMatchBase):
    title = "EdgeMetadataIxbarRead"
    description = "Read after write input XBAR read dependency."

    sub_type = jsl.StringField(
        required=True, enum=["ixbar_read"], description="Type of match dependency."
    )
    dep_fields = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("FieldSlice", as_ref=True),
        description="Dependency fields.",
    )


# Action Type Edges
class EdgeMetadataActionBase(EdgeMetadataBase):
    title = "EdgeMetadataActionBase"
    description = "Common definitions for action type edge metadata"

    type = jsl.StringField(required=True, enum=["action"], description="Type of dependency.")


class EdgeMetadataActionRead(EdgeMetadataActionBase):
    title = "EdgeMetadataActionRead"
    description = "Action read after action write dependency."

    sub_type = jsl.StringField(
        required=True, enum=["action_read"], description="Type of action dependency."
    )
    action_name = jsl.StringField(required=False, description="The name of the action.")
    dep_fields = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("FieldSlice", as_ref=True),
        description="Dependency fields.",
    )


class EdgeMetadataOutput(EdgeMetadataActionBase):
    title = "EdgeMetadataOutput"
    description = "Write after write output dependency."

    sub_type = jsl.StringField(
        required=True, enum=["output"], description="Type of action dependency."
    )
    dep_fields = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("FieldSlice", as_ref=True),
        description="Dependency fields.",
    )


class EdgeMetadataContainerConflict(EdgeMetadataActionBase):
    title = "EdgeMetadataContainerConflict"
    description = "Shared containers introducing table ordering dependency."

    sub_type = jsl.StringField(
        required=True, enum=["cont_conflict"], description="Type of action dependency."
    )
    phv_number = jsl.IntField(required=True, description="Unique address of the PHV container.")


# Reduction Or Edges
class EdgeMetadataReductionOr(EdgeMetadataBase):
    title = "EdgeMetadataReductionOr"
    description = "Reduction Or dependency"

    type = jsl.StringField(required=True, enum=["reduction_or"], description="Type of dependency.")
    sub_type = jsl.StringField(
        required=True,
        enum=["reduction_or_read", "reduction_or_output"],
        description="Type of data dependency.",
    )
    dep_fields = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("FieldSlice", as_ref=True),
        description="Dependency fields.",
    )


class FieldSlice(jsl.Document):
    title = "FieldSlice"
    description = "A slice of a field."
    additional_properties = False

    gress = jsl.StringField(
        required=True, enum=["ingress", "egress", "ghost"], description="Gress of this field."
    )
    field_name = jsl.StringField(
        required=True,
        description="Name of this field (should be in the same format as context.json)",
    )
    start_bit = jsl.IntField(required=True, description="Least significant bit of the field slice.")
    width = jsl.IntField(required=True, description="Size (in bits) of the field slice.")

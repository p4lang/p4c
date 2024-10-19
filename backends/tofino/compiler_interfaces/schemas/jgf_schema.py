#!/usr/bin/env python3

"""
jgf_schema.py: Generates the JSON Graph Format schema and provides the schema in JSL
format so that other JSL documents may extend this schema.
http://jsongraphformat.info/
"""

import json

import jsl

########################################################
#   Schema Version
########################################################

"""
Version notes:
1.0.0
- Initial version of this schema based on http://jsongraphformat.info/
"""

major_version = 1
revision_version = 0  # (medium version)
addition_version = 0  # (minor version)


def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(revision_version), str(addition_version))


########################################################


class JsonGraphFormatSchema(jsl.Document):
    title = "JsonGraphFormatSchema"
    description = "Generic JSON Graph Format schema (http://jsongraphformat.info/)"
    additional_properties = False

    label = jsl.StringField(required=False, description="Graph collection label.")
    type = jsl.StringField(required=False, description="Graph collection type.")
    metadata = jsl.OneOfField(
        [jsl.DictField(), jsl.NullField()],
        required=False,
        description="Graph collection metadata object.",
    )
    graphs = jsl.ArrayField(
        required=True,
        items=jsl.DocumentField("Graph", as_ref=True, required=True),
        description="An array of JSON graphs.",
    )


class Graph(jsl.Document):
    title = "Graph"
    description = "JGF Graph object"
    additional_properties = False

    label = jsl.StringField(required=False, description="Graph label.")
    directed = jsl.BooleanField(
        required=False, description="Directed or undirected graph.", default=True
    )
    type = jsl.StringField(required=False, description="Graph type.")
    metadata = jsl.OneOfField(
        [jsl.DictField(), jsl.NullField()], required=False, description="Graph metadata object."
    )
    nodes = jsl.OneOfField(
        [
            jsl.ArrayField(jsl.DocumentField("GraphNode", as_ref=True, description="Graph node.")),
            jsl.NullField(),
        ],
        required=False,
        description="List of all graph nodes.",
    )
    edges = jsl.OneOfField(
        [
            jsl.ArrayField(jsl.DocumentField("GraphEdge", as_ref=True, description="Graph edge.")),
            jsl.NullField(),
        ],
        required=False,
        description="List of all graph edges.",
    )


class GraphNode(jsl.Document):
    title = "GraphNode"
    description = "JGF Node object"
    additional_properties = False

    id = jsl.StringField(required=True, description="Unique identifier for this graph node.")
    label = jsl.StringField(required=False, description="Graph node label.")
    metadata = jsl.OneOfField(
        [jsl.DictField(), jsl.NullField()], required=False, description="Graph node metadata object"
    )


class GraphEdge(jsl.Document):
    title = "GraphEdge"
    description = "JGF Edge object"
    additional_properties = False

    id = jsl.StringField(required=False, description="Unique identifier for this graph edge.")
    label = jsl.StringField(required=False, description="Graph edge label.")
    source = jsl.StringField(required=True, description="The ID of the source node object.")
    target = jsl.StringField(required=True, description="The ID of the target node object.")
    relation = jsl.StringField(
        required=False, description="The interaction between the source and target node."
    )
    directed = jsl.OneOfField(
        [
            jsl.BooleanField(default=True),
            jsl.NullField(),
        ],
        required=False,
        description="Directed or undirected edge mode. Determined by graph if omitted.",
    )
    metadata = jsl.OneOfField(
        [jsl.DictField(), jsl.NullField()],
        required=False,
        description="Graph edge metadata object.",
    )

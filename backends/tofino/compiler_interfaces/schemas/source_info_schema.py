#!/usr/bin/env python3

"""
source_schema.py: Generates JSON schema for source info
"""

import jsl
import json

########################################################
#   Schema Version
########################################################

"""
1.0.0
Initial version
"""

# versioning follows SchemaVer convention
major_version    = 1
revision_version = 0
addition_version = 0

def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(revision_version), str(addition_version))

########################################################

class Source_infoJSONSchema(jsl.Document):
    title = "SourceInfoSchema"
    description = "Source info schema definition"

    schema_version = jsl.StringField(required=True, enum=[get_schema_version()], description="Schema version used to produce this JSON.")
    source_root = jsl.StringField(required=False, description="The root folder of the source tree.")
    symbols = jsl.ArrayField(required=True, items=jsl.DocumentField("SymbolDefinition", as_ref=True), description="An array of source symbols.")

class SymbolDefinition(jsl.Document):
    title = "SymbolDefinition"
    description = "Source symbol definition object"

    symbol_type = jsl.StringField(required=False, description="The type of this symbol (e.g. p4 table, condition, etc.)")
    symbol_name = jsl.StringField(required=True, description="Full name of this source symbol.")
    scope = jsl.ArrayField(required=False, items=jsl.StringField(), description="Symbol scope path as an array.")
    aliases = jsl.ArrayField(required=False, items=jsl.StringField(), description="Fully qualified symbol name aliases.")
    declaration = jsl.DocumentField("SourceLocation", as_ref=True, description="Declaration of this symbol.")
    references = jsl.ArrayField(required=True, items=jsl.DocumentField("SourceLocation", as_ref=True), description="An array of references to this symbol.")

class SourceLocation(jsl.Document):
    title = "SourceLocation"
    description = "Represents a location in a file"

    file = jsl.StringField(required=True, description="Path to the file relative to source_root.")
    line = jsl.IntField(required=True, description="Line number in file.")
    column = jsl.IntField(required=False, description="Position in file line.")
    end_line = jsl.IntField(required=False, description="End line (inclusive) for multi-line references.")
    source_brief = jsl.StringField(required=False, description="Short summary of this source location.")

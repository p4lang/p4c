#!/usr/bin/env python3

"""
phv_schema.py: Generates a JSON Schema model for structured PHV allocation result information.
TODO:
1. Container container_type cannot be a clot. Need to figure out how to
   represent clot info
2. Need to model "tagalong" collections for resources group_type.
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

1.0.1:
- Initial version migrated.
1.0.2:
- Add nStages to indicate number of MAU stages.
- Add bit_width attribute to resources node.
1.0.3:
- Add 'target' attribute.
2.0.0:
- Add field, constraints, and allocation information
2.0.1:
- Add mutually_exclusive_with optional list to ContainerSlice
2.1.0:
- Add reason_type to ConstraintReason classes
2.1.1:
- Add gress field to Containers
2.1.2:
- Add gress field to Strutures
2.1.3:
- Allow empty field_slices and phv_slices in Field
3.0.0:
- Refactor constraint into couple of base classes
- Based on this document: https://intel.sharepoint.com/:w:/r/sites/BXD/_layouts/15/Doc.aspx?sourcedoc=%7B5420bee5-6d27-4c12-8ea4-8fe5b3a5b9ac%7D
"""

major_version = 3
medium_version = 0
minor_version = 0

def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(medium_version), str(minor_version))

########################################################

# ------------------------------
#  Location information
# ------------------------------
class Location(jsl.Document):
    title ="Location"
    description = "Base class for different program points."
    type = jsl.StringField(required=True, enum=["mau", "parser", "deparser"])

class SourceLocation(jsl.Document):
    title = "SourceLocation"
    description = "Represents a program point file name, line number."
    file = jsl.StringField(required=True, description="The name of the file (full path).")
    line = jsl.IntField(required=True, description="Line number in file.")

class MAULocation(Location):
    title = "MAULocation"
    description = "MAU program location: table, action, stage, and type of access."
    stage = jsl.IntField(required=True, description="The stage number.")
    # more specific, e.g. gateway xbar, stateful input xbar?
    detail = jsl.StringField(required=True, enum=["xbar", "vliw"])
    table = jsl.StringField(required=False, description="The table name of the location.")
    action = jsl.StringField(required=False, description="The action name of the location.")

class ParserLocation(Location):
    title = "ParserLocation"
    description = "Parser program location: parser state and type of access."
    state = jsl.StringField(required=True, description="The name of the parser state.")
    detail = jsl.StringField(required=True, enum=["ibuf", "const"])

class DeparserLocation(Location):
    title = "DeparserLocation"
    description = "Deparser program location. Defines the type access at the deparser."
    detail = jsl.StringField(required=True,
                             enum=["pkt", "mirror_digest", "resubmit_digest", "learning_digest", "pktgen", "bridge", "imeta", "checksum", "pov"])

# ATTENTION: if you add a new location source, also add to the enum in the location class.


# ---------------------------------
#  Slices: fields or phv containers
# ---------------------------------
class Slice(jsl.Document):
    title = "Slice"
    description = "Represents a slice of a field or container, a contiguous bit range [msb:lsb] (big endian order)."
    lsb = jsl.IntField(required=True, description="Least significant bit of the field slice or position in the container.")
    msb = jsl.IntField(required=True, description="Most significant bit of this field slice or position in the container.")

class Access(jsl.Document):
    title = "Access"
    description = "The location in which a field is accessed."
    location = jsl.OneOfField([
            jsl.DocumentField("MAULocation", as_ref=True),
            jsl.DocumentField("ParserLocation", as_ref=True),
            jsl.DocumentField("DeparserLocation", as_ref=True)],
            required=True,
            description="The location where this access is occuring.")

class FieldSlice(jsl.Document):
    title = "FieldSlice"
    description = "Represents a field slice."
    field_name = jsl.StringField(required=True, description="Name of the field to which this slice belongs to.")
    slice_info = jsl.DocumentField("Slice", as_ref=True, required=True, description="A slice range for this field slice.")

class ContainerSlice(jsl.Document):
    title = "ContainerSlice"
    description = "Represents information about an allocated slice related to a field."
    slice_info = jsl.DocumentField("Slice", as_ref=True, required=True, description="A slice range for this container slice.")
    field_slice = jsl.DocumentField("FieldSlice", as_ref=True, required=True,
                                    description="Reference to field slice that this container slice represents.")
    phv_number = jsl.IntField(required=True, description="Unique address of this PHV container.")
    mutually_exclusive_with = jsl.ArrayField(required=False, items=jsl.StringField(required=True),
                                        description="List of field names in the same container which are mutually exclusive with this field.")

    # access patterns
    reads = jsl.ArrayField(required=True, items=jsl.DocumentField("Access", as_ref=True),
                           description="A list of locations where this field is read.")
    writes = jsl.ArrayField(required=True, items=jsl.DocumentField("Access", as_ref=True),
                           description="A list of locations where this field is written.")

# ------------------------------
#  Container information
# ------------------------------

class Container(jsl.Document):
    title = "PHV Container"
    description = "PHV container information."

    phv_number = jsl.IntField(required=True, description="Unique address of this PHV container.")
    bit_width = jsl.IntField(required=True, enum=[8, 16, 32], description="The PHV container bit width.")
    phv_mau_group_number = jsl.IntField(required=True, description="MAU group ID.")
    phv_deparser_group_number = jsl.IntField(required=True, description="Deparser group ID.")
    container_type = jsl.StringField(required=True, enum=["normal", "tagalong", "mocha", "dark"],
                                     description="A string describing the type of this PHV container.")
    gress = jsl.StringField(required=True,
                            enum=["ingress", "egress", "ghost"],
                            description="The packet thread.")
    slices = jsl.ArrayField(required=True, items=jsl.DocumentField("ContainerSlice", as_ref=True), min_items=1,
                             description="A list of container slices occupying this container.")

# --------------------------------------------------------------
#  Header layout structures
# (including compiler-generated, e.g. phase 0, bridged metadata)
# --------------------------------------------------------------
class Structure(jsl.Document):
    title = "Structure"
    description = "Field group structures."

    name = jsl.StringField(required=True,
                           description="The name of this structure.  For example, a header instance name.")
    type = jsl.StringField(required=True,
                           enum=["header", "bridge", "mirror_digest", "phase0", "learning_digest", "resubmit_digest"],
                           description="The type of this structure.")
    gress = jsl.StringField(required=True,
                            enum=["ingress", "egress", "ghost"],
                            description="The packet thread.")
    ordered_fields = jsl.ArrayField(required=True, items=jsl.DocumentField("FieldSlice", as_ref=True), min_items=1,
                                  description="An ordered list of fields in the structure,"
                                              "from most significant field to least significant.")

# ------------------------------
#  Constraints
# ------------------------------

class BaseConstraintType(jsl.StringField):
    description = """A type of constraint field."""
    _FORMAT = 'constraint_type' # For whatever reason gets serialized as "reason_type"

# NOTE: Underscore in the name is not a typo. It is required in order for
# generate_logging.py to generate proper C++.
class Base_Constraint(jsl.Document):
    class Options(object):
        definition_id = "Base_Constraint"
        inheritance_mode = jsl.ALL_OF
        additional_properties = True
    description = """A base class for definining generic constraint classes applied to a field."""
    name = jsl.StringField(required=True, description="Name of the constraint used for filtering")
    id = jsl.IntField(description="Id for the constraint, used to retrieve reason message from constraint_reasons", required=True)
    source = jsl.DocumentField("SourceLocation", as_ref=True, required=True,
                               description="The source of the constraint.")
    pipeline_location = jsl.DocumentField("Access", as_ref=True, required=False,
                               description="The location where this constraint is occuring.")

class BoolConstraint(Base_Constraint):
    class Options(object):
        definition_id = "BoolConstraint"
        inheritance_mode = jsl.ALL_OF

    description = """A constraint which is always enforced. If inverted, it can enforce exact
                     opposite behaviour (must be solitary vs must be shared)."""
    constraint_type = BaseConstraintType("BoolConstraint")
    inverted = jsl.BooleanField(required=True, description="If set to true, constraint enforces exact opposite behaviour")

class IntConstraint(Base_Constraint):
    class Options(object):
        definition_id = "IntConstraint"
        inheritance_mode = jsl.ALL_OF

    description = """A constraint with a single integer parameter (offset, stage num, container size)."""
    constraint_type = BaseConstraintType("IntConstraint")
    value = jsl.IntField(required=True, description="Parameter of the constraint")

class ListConstraint(Base_Constraint):
    class Options(object):
        definition_id = "ListConstraint"
        inheritance_mode = jsl.ALL_OF

    description = "Constraint applied to set of lists of fields."
    constraint_type = BaseConstraintType("ListConstraint")
    lists = jsl.ArrayField(required=True, items=jsl.IntField(required=True, description="Index into field_items array"), min_items=1)

class Constraint(jsl.Document):
    title="Constraint"
    description="A constraint on a field slice."
    field_slice = jsl.DocumentField("FieldSlice", as_ref=True, required=False,
                                    description="The field slice to which the constraint applies.")
    base_constraint = jsl.ArrayField(required=True, items=jsl.OneOfField([
            jsl.DocumentField('BoolConstraint', as_ref=True),
            jsl.DocumentField('IntConstraint', as_ref=True),
            jsl.DocumentField('ListConstraint', as_ref=True)]),
        min_items=1, description="The reason(s) the constraint is attached to the field. A constraint may be imposed by a multitude of reasons"
                                 "each coming from a different source. In this array, all such reasons should be listed.")


# ------------------------------
#  Field, FieldInfo, FieldPair
# ------------------------------

class FieldInfo(jsl.Document):
    title = "FieldInfo"
    description = """Common information to identify a field for both a P4 field object
                     (as a set of slices) and as an element in a physical container."""

    field_name = jsl.StringField(required=True, description="Name of the field.")
    field_class = jsl.StringField(required=True,
                                  enum=["pkt", "meta", "imeta", "pov", "bridged", "padding", "compiler_added"],
                                  description="A classification designation for this field.")
    format_type = jsl.StringField(required=False,
                                  description="A user-supplied type for this field, which can be"
                                              "used for string formatting during display.")
    gress = jsl.StringField(required=True,
                            enum=["ingress", "egress", "ghost"],
                            description="The packet thread.")
    bit_width = jsl.IntField(required=True,
                             description="The P4 field bit width. For variable bit fields, this is the maximum width.")

class FieldGroupItem(jsl.Document):
    title = "FieldGroupItem"
    field_info = jsl.DocumentField("FieldInfo", as_ref=True, required=True, description="Reference to first field in pair")
    source = jsl.DocumentField("SourceLocation", as_ref=True, required=True, description="Program points inducing the constraint.")
    # I would like to have here either pointer to field slice or nullptr explicitly, but that is not possible to serialize to cpp
    slice = jsl.DocumentField("Slice", as_ref=True, required=False, description="Field slice. All slices of a field if not present")

class Field(jsl.Document):
    title = "Field"
    description = "Represents a P4 field with slicing information, access patterns, constraints, and allocation status."

    field_info = jsl.DocumentField("FieldInfo", as_ref=True, required=True, description="Field identification info.")

    field_slices = jsl.ArrayField(required=True, items=jsl.DocumentField("FieldSlice", as_ref=True), min_items=0,
                                  description="A list of field slices comprising this field.")

    phv_slices = jsl.ArrayField(required=True, items=jsl.DocumentField("ContainerSlice", as_ref=True), min_items=0,
                                      description="A list of container slices comprising this field.")

    constraints = jsl.ArrayField(required=True, items=jsl.DocumentField("Constraint", as_ref=True), min_items=0,
                                 description="An array of Constraint objects for the field."
                                             "A field may have multiple constraints, due to multiple uses."
                                             "The constraints field is required, however, it is allowed to be an empty array.")

    structure = jsl.StringField(required=False, description="Structure name defining this field.")

    status = jsl.StringField(required=True, enum=["allocated", "unallocated", "partially allocated", "unreferenced", "eliminated"],
                            description="The state in which the field is at the time of logging.")

# ------------------------------
#  Top level JSON schema
# ------------------------------
class PhvJSONSchema(jsl.Document):
    title = "PhvJSONSchema"
    description = "PHV JSON schema for a P4 program's PHV allocation results.."
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    build_date = jsl.StringField(required=True, description="Timestamp of when the program was built.")
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    compiler_version = jsl.StringField(required=True, description="Compiler version used in compilation.")
    schema_version = jsl.StringField(required=True, enum=[get_schema_version()], description="Schema version used to produce this JSON.")

    # hardware info: chip, MAU and PHV
    target = jsl.StringField(required=True, enum=TARGETS,
                             description="The target device this program was compiled for.")

    nStages = jsl.IntField(required=True, description="The number of MAU stages.")

    resources = jsl.ArrayField(items=jsl.DictField(properties={
        "group_type": jsl.StringField(required=True, enum=["mau", "tagalong", "deparser"], description="The PHV group type."),
        "bit_width": jsl.IntField(required=True, description="The bit width of containers in this group."),
        "num_containers": jsl.IntField(required=True, description="The number of PHV containers in this group."),
        "addresses": jsl.ArrayField(items=jsl.IntField(required=True,
                                                       description="The starting address of PHV containers in this group."
                                                                   "Groups are sequences of containers."),
                                    required=True, description="The container addresses in this group.")
    }), required=True, description="A list of PHV resource information.")

    # P4-level information on headers, structs, field lists, overlays, etc.
    structures = jsl.ArrayField(items=jsl.DocumentField("Structure", as_ref=True), required=True,
                                description="A list of program and compiler-induced structures,"
                                            "e.g. header structures and bridged metadata.")
    # all program fields
    fields = jsl.ArrayField(required=True, items=jsl.DocumentField("Field", as_ref=True),
                            description="The list of fields in the program. The fields may be in different stages,"
                                        "depending on the result of the PHV allocation.")

    # Concrete items indexed by field_groups
    field_group_items = jsl.ArrayField(items=jsl.DocumentField("FieldGroupItem", required=True, as_ref=True),
        required=False, description="Concrete items pointed to by indices in field_groups")

    # Groups of fields affected by same constraints
    field_groups = jsl.ArrayField(
        items=jsl.ArrayField(
            items=jsl.IntField(required=True, description="Index to field_group_items"), required=True,
            min_items=2, description="Group of fields under same constraint"),
        required=False, description="Groups of fields affected by the same constraints")

    # Constraint reasons. Maps generic string id to formatted reason string
    constraint_reasons = jsl.ArrayField(required=True, items=jsl.StringField(required=True), min_items = 1,
                            description="Array of reasons for various types and subtypes of constraints. Indexed by constraint id")

    # POVs
    pov_structure = jsl.ArrayField(items=jsl.DictField(properties={
        "pov_bit_index": jsl.IntField(required=True, description="Bit index in the packet occupancy vector."),
        "pov_bit_name": jsl.StringField(required=True, description="The name given to the POV bit."),
        "gress": jsl.StringField(required=True, enum=["ingress", "egress", "ghost"], description="The gress of the validity bit.")
    }), required=True, description="A list of attributes describing the POV structure.")

    # the allocation
    containers = jsl.ArrayField(items=jsl.DocumentField("Container", as_ref=True), required=True,
                                description="A list of PHV container allocation information.")

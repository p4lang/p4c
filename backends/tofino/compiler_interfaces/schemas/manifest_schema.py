#!/usr/bin/env python3

"""
manifest_schema.py: Defines the JSON schema for the compiler archive manifest.

Schema versions:
1.0.0 - initial schema
1.1.0 - add pipe_name to contexts node
1.2.0 - add command line arguments
1.3.0 - add resources files
1.3.1 - add type to resource files
1.4.0 - add architecture description object
1.5.0 - add compilation_succeeded attribute, which indicates if contents of archive is complete.
1.5.1 - add Tofino2 variants: tofino2h, tofino2m, tofino2u
1.5.2 - add gress and pipe to graphs nodes
2.0.0 - reorganize compiler outputs per pipe
          - add compilation time
          - add driver conf file
2.0.1 - add a few more attributes to log and graph types to support p4c-tofino generated ouputs
2.0.2 - add metrics json path
2.0.3 - add source file references
2.0.4 - add source.json reference
2.0.5 - add event log file path
2.0.6 - add frontend ir log file path
3.0.0 - update Manifest structure to support multiple target platforms (universal manifest)
3.0.1 - made the per-pipe "compilation_succeeded" be optional
"""

import inspect
import json
import os.path
import sys

import jsl

MYPATH = os.path.dirname(__file__)
if not getattr(sys, 'frozen', False):
    # standalone script
    SCHEMA_PATH = os.path.join(MYPATH, "../")
    sys.path.append(SCHEMA_PATH)

if os.path.exists(os.path.join(MYPATH, 'targets.py')):
    from schemas.targets import P4ARCHITECTURES, TARGETS
else:
    TARGETS = ['tofino', 'tofino2', 'tofino2h', 'tofino2m', 'tofino2u']
    P4ARCHITECTURES = ['tna', 't2na', 'psa', 'PISA', 'v1model']

major_version = 3
minor_version = 0
patch_version = 1


def get_schema_version():
    return "%s.%s.%s" % (str(major_version), str(minor_version), str(patch_version))


#### ENTRY FORMATS ####
class BFNCompilerArchive(jsl.Document):
    title = "BFNCompilerArchive"
    description = "Manifest file for a compiler archive"
    schema_version = jsl.StringField(required=True, description="Manifest schema version")
    target = jsl.StringField(required=True, description="Target device", enum=TARGETS)

    build_date = jsl.StringField(
        required=True, description="Timestamp of when the archive was built."
    )
    compiler_version = jsl.StringField(
        required=True, description="Compiler version used in compilation."
    )
    compile_command = jsl.StringField(required=False, description="Compiler command line")
    compilation_succeeded = jsl.BooleanField(
        required=True, description="A Boolean indicating if the compilation succeeded."
    )
    compilation_time = jsl.StringField(
        required=True, description="The time (sec) it took to compile the program."
    )
    run_id = jsl.StringField(required=True, description="Unique ID for this compile run.")
    target_data = jsl.DocumentField(
        "TargetData", as_ref=True, required=True, description="Target dependant data"
    )
    programs = jsl.ArrayField(
        required=True,
        description="Array of compiled programs",
        items=jsl.DocumentField("CompiledProgram", as_ref=True),
    )


# TARGET DEPENDENT
class TargetData(jsl.Document):
    architecture = jsl.StringField(enum=P4ARCHITECTURES, required=True)
    architectureConfig = jsl.DocumentField("TofinoArchitecture", as_ref=True, required=False)
    conf_file = jsl.StringField(
        required=False, description="Path to the compiler generated driver conf file"
    )


class TofinoArchitecture(jsl.Document):
    title = "TofinoArchitecture"
    description = "Tofino multipipeline configuration definition"
    name = jsl.StringField(required=True, description="configuration name")
    pipes = jsl.ArrayField(
        required=True,
        description="list of pipelines in the program",
        items=jsl.DictField(
            properties={
                "pipe": jsl.IntField(required=True, description="logical pipe number"),
                "ingress": jsl.DocumentField(
                    "PipelineConfig", description="ingress configuration", required=True
                ),
                "egress": jsl.DocumentField(
                    "PipelineConfig", description="egress configuration", required=True
                ),
            },
            required=True,
            description="",
            additional_properties=False,
        ),
        min_items=1,
    )


class PipelineConfig(jsl.Document):
    title = "PipelineConfig"
    description = "Represent how the pipeline is connecting different controls"
    pipeName = jsl.StringField(required=True, description="Control name")
    nextControl = jsl.ArrayField(
        required=True,
        description="list of possible next controls",
        items=jsl.DictField(
            properties={
                "pipe": jsl.IntField(required=True, description="logical next pipe number"),
                "pipeName": jsl.StringField(required=True, description="next control name"),
            },
            required=True,
            description="",
            additional_properties=False,
        ),
    )


class InputFiles(jsl.Document):
    title = "InputFiles"
    description = "Summary of the source input files"
    src_map = jsl.StringField(required=False, description="Path to source.json file")
    src_root = jsl.StringField(required=True, description="Path to root source directory")
    includes = jsl.ArrayField(
        jsl.StringField(), required=False, description="List of include directories"
    )
    defines = jsl.ArrayField(
        jsl.StringField(), required=False, description="List of pre-processor defines"
    )


# TARGET DEPENDENT
class TargetOutputFiles(jsl.Document):
    title = "OutputFiles"
    description = " All the outputs of the compiler for a single pipe"
    context = jsl.DictField(
        required=True,
        properties={
            "path": jsl.StringField(required=True, description="Path to the context.json file")
        },
    )
    resources = jsl.ArrayField(
        required=False,
        description="Array of per pipe resource characterization files",
        items=jsl.DictField(
            required=True,
            properties={
                "path": jsl.StringField(required=True, description="Path to the json file"),
                "type": jsl.StringField(
                    required=True,
                    description="Type of resources",
                    enum=['parser', 'phv', 'power', 'resources', 'table'],
                ),
            },
        ),
    )
    # not needed for visualization, but needed if we go with a zip file
    binary = jsl.DictField(
        required=False,
        properties={"path": jsl.StringField(required=True, description="Path to the binary file")},
    )
    graphs = jsl.ArrayField(
        required=False,
        description="List of graph files",
        items=jsl.DictField(
            required=False,
            properties={
                "graph_type": jsl.StringField(
                    required=True, description="Type of graph", enum=['parser', 'control', 'table']
                ),
                # Right now we support only .dot files, in the future we may want other formats
                "graph_format": jsl.StringField(
                    required=True, description="The format for the file."
                ),
                "gress": jsl.StringField(
                    required=True, description="Pipeline gress", enum=['ingress', 'egress', 'ghost']
                ),
                "path": jsl.StringField(required=True, description="Path to the graph file"),
            },
        ),
    )
    logs = jsl.ArrayField(
        required=False,
        description="List of log files",
        items=jsl.DictField(
            required=False,
            properties={
                "log_type": jsl.StringField(
                    required=True,
                    description="Type of log",
                    enum=['parser', 'phv', 'mau', 'power', 'text'],
                ),
                "path": jsl.StringField(required=True, description="Path to the log file"),
            },
        ),
    )
    metrics = jsl.DictField(
        required=False,
        description="Metrics file info",
        properties={"path": jsl.StringField(required=True, description="Path to the metrics file")},
    )


class CompiledProgram(jsl.Document):
    title = "CompiledProgram"
    description = "Compiled program properties"
    program_name = jsl.StringField(required=True, description="Name of the compiled program.")
    p4_version = jsl.StringField(
        required=True, description="P4 version of program", enum=['p4-14', 'p4-16']
    )
    p4runtime_file = jsl.StringField(required=False, description="Path to the p4runtime file")
    event_log_file = jsl.StringField(required=False, description="Path to Event log file")
    frontend_ir_log_file = jsl.StringField(
        required=False, description="Path to frontend IR log file"
    )
    source_files = jsl.DocumentField("InputFiles", as_ref=True, required=True)
    # TARGET DEPENDENT
    pipes = jsl.ArrayField(
        required=True,
        description="An array of output files for each pipe",
        min_items=1,
        items=jsl.DictField(
            required=True,
            properties={
                "pipe_id": jsl.IntField(
                    required=True, description="Logical id of the control flow"
                ),
                "pipe_name": jsl.StringField(
                    required=True, description="Control flow name from P4 program"
                ),
                "files": jsl.DocumentField("TargetOutputFiles", as_ref=True, required=True),
                "compilation_succeeded": jsl.BooleanField(
                    required=False,
                    description="A Boolean indicating if the individual pipe compilation succeeded.",
                ),
            },
        ),
    )

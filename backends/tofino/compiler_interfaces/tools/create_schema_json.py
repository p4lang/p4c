#!/usr/bin/env python3

import json
import os
import shutil
import sys

if not getattr(sys, 'frozen', False):
    MYPATH = os.path.dirname(__file__)
    SCHEMA_PATH = os.path.normpath(os.path.join(MYPATH, "../"))
    sys.path.append(SCHEMA_PATH)

output = "./generated/"
# remove output dir, then create empty output dir
shutil.rmtree(output, ignore_errors=True)
os.mkdir(output)


# This function will generate a schema json with the given filename
def write_schema(filename, schema_document):
    with open(filename, 'w') as f:
        json.dump(schema_document.get_schema(ordered=True), f, indent=2, separators=(',', ':'))


# For each python-jsl schema generate a versioned JSON schema file.
# One use case for this is for generating Typescript definitions for
# P4 Insight (using tools like json2ts).

from schemas.manifest_schema import BFNCompilerArchive
from schemas.manifest_schema import get_schema_version as get_manifest_ver

write_schema('%s%s-%s.json' % (output, 'manifest', get_manifest_ver()), BFNCompilerArchive)

from schemas.context_schema import ContextJSONSchema
from schemas.context_schema import get_schema_version as get_context_ver

write_schema('%s%s-%s.json' % (output, 'context', get_context_ver()), ContextJSONSchema)

from schemas.resources_schema import ResourcesJSONSchema
from schemas.resources_schema import get_schema_version as get_resources_ver

write_schema('%s%s-%s.json' % (output, 'resources', get_resources_ver()), ResourcesJSONSchema)

from schemas.phv_schema import PhvJSONSchema
from schemas.phv_schema import get_schema_version as get_phv_ver

write_schema('%s%s-%s.json' % (output, 'phv', get_phv_ver()), PhvJSONSchema)

from schemas.mau_schema import MauJSONSchema
from schemas.mau_schema import get_schema_version as get_mau_ver

write_schema('%s%s-%s.json' % (output, 'mau', get_mau_ver()), MauJSONSchema)

from schemas.source_info_schema import Source_infoJSONSchema
from schemas.source_info_schema import get_schema_version as get_source_ver

write_schema('%s%s-%s.json' % (output, 'source', get_source_ver()), Source_infoJSONSchema)

from schemas.power_schema import PowerJSONSchema
from schemas.power_schema import get_schema_version as get_power_ver

write_schema('%s%s-%s.json' % (output, 'power', get_power_ver()), PowerJSONSchema)

from schemas.table_graph_schema import TableGraphsSchema
from schemas.table_graph_schema import get_schema_version as get_tg_ver

write_schema('%s%s-%s.json' % (output, 'table-graph', get_tg_ver()), TableGraphsSchema)

from schemas.event_log_schema import Event_logJSONSchema
from schemas.event_log_schema import get_schema_version as get_el_ver

write_schema('%s%s-%s.json' % (output, 'event-log', get_el_ver()), Event_logJSONSchema)

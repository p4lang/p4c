import json
import sys
from jsonschema import validate

json_file = sys.argv[1]
schema_file = sys.argv[2]

print "Validating `%s` JSON file against `%s` schema..." % (json_file, schema_file)

j  = {}
with open(json_file, 'r') as f:
    j = json.load(f)

s = {}
with open(schema_file, 'r') as f:
    s = json.load(f)

try:
    validate(j, s)
except Exception, e:
    print "Failed to validate JSON file against schema."
    print str(e)
    exit(1)

print "File validated against schema."

#! /usr/bin/env python3
# Base class to validate json schema files.
#
# The class constructor initializes arguments, including the expected
# .json file to be validated.
#
# The validation routine
#
# The script can optionally dump the schema into a file.
#
# \TODO: add support for passing a generated schema as an argument.

import argparse
import json
import os
import sys

import jsonschema


class JSONValidator:
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument(
            "--schema", action="store", default=None, help="load this schema instead of the default"
        )
        parser.add_argument(
            "--dump-schema", action="store", default=None, help="dump the schema into file"
        )
        parser.add_argument("-g", dest="debug", action="store_true", default=False, help="debug")
        parser.add_argument("generated_file", help="the generated json file to validate")
        self._opts = parser.parse_args()
        if self._opts.schema is not None:
            print("explicit schema not yet supported")
            sys.exit(1)

    def validate(self, pretty_name, schema_generator, schema_version):
        """validates the schema in encoded in the schema_generator"""
        try:
            if self._opts.debug:
                print("validating", self._opts.generated_file)

            schema = schema_generator.get_schema()
            target_file = json.load(open(self._opts.generated_file, 'r'))

            if target_file['schema_version'] != schema_version:
                print(
                    "Invalid",
                    pretty_name,
                    "schema version",
                    target_file['schema_version'],
                    file=sys.stderr,
                )
                print("Attempted validation against", schema_version, file=sys.stderr)
                return False

            v = jsonschema.Draft4Validator(schema)
            errors = sorted(v.iter_errors(target_file), key=lambda e: e.path)

            if errors:
                print(pretty_name + " schema validation failed:")

                for error in errors:
                    print("error path: ")
                    print(list(error.path))
                    print("error: ", error.message)
                    print("More details: ")
                    for suberror in sorted(error.context, key=lambda e: e.schema_path):
                        print(list(suberror.schema_path), suberror.message, sep=", ")

                return False
            else:
                if self._opts.debug:
                    print("successful validation")

                return True

        except Exception as e:
            print(e)
            return False

    def dump_schema(self, output_schema, schema_generator):
        """Outputs the schema from schema_generator to the output_schema file"""
        try:
            if output_schema is not None:
                schema = schema_generator.get_schema()
                with open(output_schema, "wb") as schema_file:
                    json.dump(schema, schema_file, indent=2)
            else:
                print("please provide a valid name for the output file")
        except e:
            print(e)

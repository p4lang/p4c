#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

SCHEMAS_DIR=$(realpath "$SCRIPT_DIR/../compiler_interfaces/schemas")

OUTPUT=""
if [ $# -ge 1 ]; then
    OUTPUT="$1"
    rm -f "$OUTPUT"
    touch "$OUTPUT" || exit $?
fi

for schema in $(basename -s ".py" "$SCHEMAS_DIR"/*.py); do
    version=$(python3 << END
import sys

sys.path.append("$SCHEMAS_DIR")

try:
    from $schema import get_schema_version
except ImportError:
    sys.exit(1)

print(get_schema_version())
END
)
    if [ $? -eq 0 ]; then
        schema_version=$(printf "%-20s : %s\n" "$schema" "$version")

        if [ "$OUTPUT" != "" ]; then
            echo "$schema_version" >> "$OUTPUT"
        else
            echo "$schema_version"
        fi
    fi
done
#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
if [[ $# != 4 || ( $1 != base && $1 != new ) ]]; then
    echo "Usage: $0 base|new BUILD_DIR FEATURE_SOURCE_DIR LOG_DIR" >&2
    exit 2
fi
mode=$1
build_dir=$2
source_dir=$3
log_dir=$4
mkdir -p "$log_dir"
log_file=$(mktemp "$log_dir/$mode-XXXXXXXX")
env -u P4TEST_REPLACE python3 "$source_dir/test/diagnostics/test_diagnostics.py" \
    --build-dir "$build_dir" --source-dir "$source_dir" --evidence-dir "$log_dir" \
    2>&1 | tee "$log_file"

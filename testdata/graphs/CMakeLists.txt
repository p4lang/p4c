# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

set(graph_inputs "testdata/core/*graph*.p4" "testdata/bmv2/v1model/*graph*.p4")
if(SUPPORT_P4_14)
  list(APPEND graph_inputs "testdata/p4_14/*graph*.p4")
endif()
p4c_add_tests("graph" ${P4C_SOURCE_DIR}/backends/graphs/run-graph-sample.sh "${graph_inputs}" "")

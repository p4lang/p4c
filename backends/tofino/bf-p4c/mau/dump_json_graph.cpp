/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/mau/dump_json_graph.h"

using namespace P4;

DumpJsonGraph::DumpJsonGraph(DependencyGraph &dg, Util::JsonObject *dgJson, cstring passContext,
                             bool placed)
    : dg(dg), dgJson(dgJson), passContext(passContext), placed(placed) {
    addPasses({new FindFlowGraph(fg)});
}

void DumpJsonGraph::end_apply(const IR::Node *) {
    if (BackendOptions().create_graphs) {
        dg.to_json(dgJson, fg, passContext, placed);
    }
}

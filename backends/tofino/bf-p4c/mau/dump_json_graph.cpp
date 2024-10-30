/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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

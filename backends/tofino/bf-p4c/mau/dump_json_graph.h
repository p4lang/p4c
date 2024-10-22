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

#ifndef BF_P4C_MAU_DUMP_JSON_GRAPH_H_
#define BF_P4C_MAU_DUMP_JSON_GRAPH_H_

#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/table_dependency_graph.h"

using namespace P4;

class DumpJsonGraph : public PassManager {
    FlowGraph fg;
    DependencyGraph &dg;
    Util::JsonObject *dgJson;
    cstring passContext;
    bool placed;

    void end_apply(const IR::Node *root) override;

 public:
    DumpJsonGraph(DependencyGraph &dg, Util::JsonObject *dgJson, cstring passContext, bool placed);
};

#endif /* BF_P4C_MAU_DUMP_JSON_GRAPH_H_ */

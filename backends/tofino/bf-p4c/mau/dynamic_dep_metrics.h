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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_DYNAMIC_DEP_METRICS_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_DYNAMIC_DEP_METRICS_H_

#include <functional>

#include "bf-p4c/mau/table_dependency_graph.h"

using namespace P4;

class DynamicDependencyMetrics {
    const CalculateNextTableProp &ntp;
    const ControlPathwaysToTable &con_paths;
    const DependencyGraph &dg;
    // FIXME -- not thread safe!
    std::function<bool(const IR::MAU::Table *)> placed_tables;

 public:
    DynamicDependencyMetrics(const CalculateNextTableProp &n, const ControlPathwaysToTable &cp,
                             const DependencyGraph &d)
        : ntp(n), con_paths(cp), dg(d) {}

    std::pair<int, int> get_downward_prop_score(const IR::MAU::Table *a,
                                                const IR::MAU::Table *b) const;

    void score_on_seq(const IR::MAU::TableSeq *seq, const IR::MAU::Table *tbl, int &max_dep_impact,
                      char type) const;
    void update_placed_tables(std::function<bool(const IR::MAU::Table *)> pt) {
        placed_tables = pt;
    }

    int total_deps_of_dom_frontier(const IR::MAU::Table *a) const;
    int placeable_cds_count(const IR::MAU::Table *tbl,
                            ordered_set<const IR::MAU::Table *> &already_placed_in_stage) const;
    bool can_place_cds_in_stage(const IR::MAU::Table *tbl,
                                ordered_set<const IR::MAU::Table *> &already_placed_in_table) const;
    double average_cds_chain_length(const IR::MAU::Table *tbl) const;
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_DYNAMIC_DEP_METRICS_H_ */

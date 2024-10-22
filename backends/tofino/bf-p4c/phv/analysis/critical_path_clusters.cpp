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

#include "bf-p4c/phv/analysis/critical_path_clusters.h"

Visitor::profile_t CalcCriticalPathClusters::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    critical_fields_i.clear();
    return rv;
}

const IR::Node *CalcCriticalPathClusters::apply_visitor(const IR::Node *node, const char *) {
    critical_fields_i = parser_critical_path.calc_all_critical_fields();
    return node;
}

ordered_set<PHV::SuperCluster *> CalcCriticalPathClusters::calc_critical_clusters(
    const std::list<PHV::SuperCluster *> &clusters) const {
    ordered_set<PHV::SuperCluster *> results;
    for (auto *super_cluster : clusters) {
        bool has_critical_field =
            std::any_of(critical_fields_i.begin(), critical_fields_i.end(),
                        [&](const PHV::Field *f) { return super_cluster->contains(f); });
        if (has_critical_field) {
            results.insert(super_cluster);
        }
    }
    return results;
}

void CalcCriticalPathClusters::print(std::ostream &out,
                                     const ordered_set<PHV::SuperCluster *> &clusters) const {
    out << "=========== Critical Clusters ===========" << std::endl;
    for (auto *super_cluster : clusters) {
        for (auto *rotational_cluster : super_cluster->clusters()) {
            for (auto *aligned_cluster : rotational_cluster->clusters()) {
                for (auto &slice : aligned_cluster->slices()) {
                    out << "Field:" << slice.field()->name << std::endl;
                }
            }
        }
        out << "=========================================" << std::endl;
    }
}

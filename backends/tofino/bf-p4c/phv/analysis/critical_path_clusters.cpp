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

#include "bf-p4c/phv/analysis/critical_path_clusters.h"

Visitor::profile_t CalcCriticalPathClusters::init_apply(const IR::Node* root) {
    auto rv = Inspector::init_apply(root);
    critical_fields_i.clear();
    return rv;
}

const IR::Node *
CalcCriticalPathClusters::apply_visitor(const IR::Node * node, const char *) {
    critical_fields_i = parser_critical_path.calc_all_critical_fields();
    return node;
}

ordered_set<PHV::SuperCluster *>
CalcCriticalPathClusters::calc_critical_clusters(
    const std::list<PHV::SuperCluster *>& clusters) const {
    ordered_set<PHV::SuperCluster *> results;
    for (auto* super_cluster : clusters) {
        bool has_critical_field = std::any_of(critical_fields_i.begin(), critical_fields_i.end(),
                [&](const PHV::Field* f) { return super_cluster->contains(f); });
        if (has_critical_field) {
            results.insert(super_cluster); } }
    return results;
}

void
CalcCriticalPathClusters::print(std::ostream& out,
                                const ordered_set<PHV::SuperCluster *>& clusters) const {
    out << "=========== Critical Clusters ===========" << std::endl;
    for (auto* super_cluster : clusters) {
        for (auto* rotational_cluster : super_cluster->clusters()) {
            for (auto* aligned_cluster : rotational_cluster->clusters()) {
                for (auto& slice : aligned_cluster->slices()) {
                    out << "Field:" << slice.field()->name << std::endl; } } }
        out << "=========================================" << std::endl; }
}

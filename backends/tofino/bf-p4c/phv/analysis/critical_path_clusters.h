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

#ifndef BF_P4C_PHV_ANALYSIS_CRITICAL_PATH_CLUSTERS_H_
#define BF_P4C_PHV_ANALYSIS_CRITICAL_PATH_CLUSTERS_H_

#include "bf-p4c/phv/analysis/parser_critical_path.h"
#include "bf-p4c/phv/make_clusters.h"
#include "ir/ir.h"
#include "lib/cstring.h"

/** Provide a function to produces an ordered_set<const SuperCluster*> that
 * those SuperClusters have at least one cluster that
 * has field(s) extracted on the parser critical path,
 * either ingress or egress. The result
 * can be fetched by getCriticalPathClusters().
 *
 * This pass does not traverse the IR tree, nor does it change states.
 * After spliting clusters, call calc_critical_clusters.
 *
 * @pre Must run after CalcParserCriticalPath
 */
class CalcCriticalPathClusters : public Inspector {
 private:
    ordered_set<const PHV::Field *> critical_fields_i;
    const CalcParserCriticalPath &parser_critical_path;
    const IR::Node *apply_visitor(const IR::Node *, const char *name = 0) override;

 public:
    explicit CalcCriticalPathClusters(const CalcParserCriticalPath &parser_critical_path)
        : parser_critical_path(parser_critical_path) {}

    ordered_set<PHV::SuperCluster *> calc_critical_clusters(
        const std::list<PHV::SuperCluster *> &clusters) const;

    profile_t init_apply(const IR::Node *root) override;

    void print(std::ostream &out, const ordered_set<PHV::SuperCluster *> &clusters) const;
};

#endif /* BF_P4C_PHV_ANALYSIS_CRITICAL_PATH_CLUSTERS_H_ */

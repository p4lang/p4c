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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_UTILS_LIVERANGE_OPTI_UTILS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_UTILS_LIVERANGE_OPTI_UTILS_H_

#include "bf-p4c/phv/analysis/dominator_tree.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "ir/ir.h"

namespace PHV {
class Transaction;
}

/// @returns true if any of the dominator units in @doms is a parser node.
bool hasParserUse(const PHV::UnitSet &doms);

/// @returns all pairs (x, y) where x is an unit in @f_units that can reach the unit y in @g_units.
ordered_set<std::pair<const IR::BFN::Unit *, const IR::BFN::Unit *>> canFUnitsReachGUnits(
    const PHV::UnitSet &f_units, const PHV::UnitSet &g_units,
    const ordered_map<gress_t, FlowGraph> &flowGraph);

/// Trim the set of dominators by removing nodes that are dominated by other dominator nodes
/// already in the set.
void getTrimmedDominators(PHV::UnitSet &candidates, const BuildDominatorTree &domTree);

/// Overlaying two fields @g and @f using ARA initialization may modify
/// the flowgraph of the program. This function updates the flowgraph
/// for initializing field @f whose min-stage based liverange is later
/// than the liverange of @g. The @return flowgraph is the original
/// flowgraph complemented with flows from all units referencing field
/// g to all units referencing field f.
/// NOTE: The updated flowgraph is meant to check for control flow
/// loops and not used as the IR flowgraph.
ordered_map<gress_t, FlowGraph> update_flowgraph(const PHV::UnitSet &g_units,
                                                 const PHV::UnitSet &f_units,
                                                 const ordered_map<gress_t, FlowGraph> &flgraphs,
                                                 const PHV::Transaction &transact, bool &canUseARA);

#endif /*  BACKENDS_TOFINO_BF_P4C_PHV_UTILS_LIVERANGE_OPTI_UTILS_H_  */

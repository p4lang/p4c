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

#ifndef BACKENDS_TOFINO_BF_P4C_IR_CONTROL_FLOW_VISITOR_H_
#define BACKENDS_TOFINO_BF_P4C_IR_CONTROL_FLOW_VISITOR_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

namespace BFN {

class ControlFlowVisitor : public ::ControlFlowVisitor, virtual public P4::ResolutionContext {
 protected:
    /** filter_join_points is only relevant for Visitors that set joinFlows = true
     * in their constructor. Most control flow visitors in the back end probably only want
     * ParserState, Table, and TableSeq join points; by default, filter all others.
     * This is relevant mostly because joinFlows does not really work properly, as flows
     * that join (node with mulitple parents) are only processed once all parents are
     * visited, *BUT* subsequent nodes (siblings after the join points) will be visited
     * immediately, *BEFORE* the join node is visited. This is, simply, wrong, but fixing
     * it requires multithreading or coroutines in the visitor infrastructure, which has not
     * yet been implemented. So some visitors need to also filter Tables (and visit them
     * repeatedly with visitDagOne = false or visitAgain) or they'll fail.
     *
     * TODO: If IR::BFN::ParserPrimitive nodes are not specifically
     * excluded from join points, then they (and their children) will be
     * visited out of control flow order.
     *
     * @warning Children overriding this method MUST filter (return true) if @n
     * is an IR::BFN::ParserPrimitive node. This is due to the visit order misfeature
     * noted above.
     */
    bool filter_join_point(const IR::Node *n) override {
        // FIXME -- we generally should NOT be filtering out Tables or TableSeqs, BUT currently
        // the joinFlows stuff doesn't work properly, as Table::visit_children does not use
        // a SplitFlowVisit to visit next fields, so it has trouble when a TableSeq is used in
        // more than one place.  It also depends on the visitor as to whether it makes more
        // sense to always revisit tables/seqs that are referenced more than once, as that
        // cam give a more accurate flow analysis (though it is more expensive)
        return !n->is<IR::BFN::ParserState>() &&
               // !n->is<IR::MAU::Table>() &&
               !n->is<IR::MAU::TableSeq>();
    }

    // Helpers for visiting midend IR in control-flow order.  These functions take a call
    // or reference to a ParserState, an Action, or a Table and visit that object.
    void visit_def(const IR::PathExpression *);
    void visit_def(const IR::MethodCallExpression *);
};

}  // end namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_IR_CONTROL_FLOW_VISITOR_H_ */

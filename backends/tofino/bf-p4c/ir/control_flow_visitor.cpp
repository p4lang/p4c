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

#include "control_flow_visitor.h"

#include "frontends/p4/methodInstance.h"

namespace BFN {

void ControlFlowVisitor::visit_def(const IR::PathExpression *pe) {
    auto *d = resolveUnique(pe->path->name, P4::ResolutionType::Any);
    BUG_CHECK(d, "failed to resolve %s", pe);
    if (auto *ps = d->to<IR::ParserState>()) {
        visit(ps, "transition");
    } else if (auto *act = d->to<IR::P4Action>()) {
        visit(act, "actions");
    } else {
        auto *obj = d->getNode();  // FIXME -- should be able to visit an INode
        visit(obj);
    }
}

void ControlFlowVisitor::visit_def(const IR::MethodCallExpression *mc) {
    auto *mi = P4::MethodInstance::resolve(mc, this);
    BUG_CHECK(mi, "failed to resolve %s", mi);
    if (auto *act = mi->to<P4::ActionCall>()) {
        visit(act->action, "action");
    } else if (mi->object) {
        auto *obj = mi->object->getNode();  // FIXME -- should be able to visit an INode
        visit(obj, "object");
    }
}

}  // namespace BFN

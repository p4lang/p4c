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

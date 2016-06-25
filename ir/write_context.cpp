/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ir.h"
#include "lib/log.h"
#include "frontends/p4/typeMap.h"

bool P4WriteContext::isWrite() {
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node)
        return false;
    if (auto *prim = ctxt->node->to<IR::Primitive>())
        return prim->isOutput(ctxt->child_index);
    if (ctxt->node->is<IR::AssignmentStatement>())
        return ctxt->child_index == 0;
    if (ctxt->node->is<IR::Vector<IR::Expression>>()) {
        if (!ctxt->parent || !ctxt->parent->node)
            return false;
        if (auto *mc = ctxt->parent->node->to<IR::MethodCallExpression>()) {
            auto type = mc->method->type->to<IR::Type_Method>();
            if (!type) {
                /* FIXME -- can't find the type of the method -- should be a BUG? */
                return true; }
            auto param = type->parameters->getParameter(ctxt->child_index);
            return param->direction == IR::Direction::Out ||
                   param->direction == IR::Direction::InOut; }
        if (ctxt->parent->node->is<IR::ConstructorCallExpression>()) {
            /* FIXME -- no constructor types?  assume all arguments are inout? */
            return true; } }
    return false;
}

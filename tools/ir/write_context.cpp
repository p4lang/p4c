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

/* Determine from the Visitor context whether the currently being visited IR node
 * denotes something that might be written to by the code.  This is always conservative
 * (we don't know for certain that it is written) since even if it is, it might be
 * writing the same value.  All you can say for certain is that if this returns 'false'
 * the code at this point will not modify the object referred to by the current node */

bool P4WriteContext::isWrite(bool root_value) {
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node)
        return root_value;
    while (ctxt->child_index == 0 &&
            (ctxt->node->is<IR::ArrayIndex>() ||
             ctxt->node->is<IR::HeaderStackItemRef>() ||
             ctxt->node->is<IR::Slice>() ||
             ctxt->node->is<IR::Member>())) {
        ctxt = ctxt->parent;
        if (!ctxt || !ctxt->node)
            return root_value; }
    if (auto *prim = ctxt->node->to<IR::Primitive>())
        return prim->isOutput(ctxt->child_index);
    if (ctxt->node->is<IR::AssignmentStatement>())
        return ctxt->child_index == 0;
    if (ctxt->node->is<IR::Argument>()) {
        // MethodCallExpression(Vector<Argument(Expression)>)
        if (!ctxt->parent || !ctxt->parent->parent || !ctxt->parent->parent->node)
            return false;
        if (auto *mc = ctxt->parent->parent->node->to<IR::MethodCallExpression>()) {
            auto type = mc->method->type->to<IR::Type_Method>();
            if (!type) {
                /* FIXME -- can't find the type of the method -- should be a BUG? */
                return true; }
            auto param = type->parameters->getParameter(ctxt->parent->child_index);
            return param->direction == IR::Direction::Out ||
                   param->direction == IR::Direction::InOut; }
        if (ctxt->parent->node->is<IR::ConstructorCallExpression>()) {
            /* FIXME -- no constructor types?  assume all arguments are inout? */
            return true; } }
    if (ctxt->node->is<IR::MethodCallExpression>()) {
        /* receiver of a method call -- some methods might be 'const' and not modify
         * their receiver, but we currently have no way of determining that */
        return true; }
    return false;
}

/* Determine from the Visitor context whether the currently being visited IR node
 * denotes something that might be read by the code.  This is often conservative
 * (we don't know for certain that it is read).  We return root_value for top-level
 * expressions, and false for expression-statement unused values.
 * TODO -- currently returns true for expressions 'read' in annotations.
 */
bool P4WriteContext::isRead(bool root_value) {
    const Context *ctxt = getContext();
    if (!ctxt || !ctxt->node)
        return root_value;
    while (ctxt->child_index == 0 &&
            (ctxt->node->is<IR::ArrayIndex>() ||
             ctxt->node->is<IR::HeaderStackItemRef>() ||
             ctxt->node->is<IR::Slice>() ||
             ctxt->node->is<IR::Member>())) {
        ctxt = ctxt->parent;
        if (!ctxt || !ctxt->node)
            return root_value; }
    if (auto *prim = ctxt->node->to<IR::Primitive>())
        return !prim->isOutput(ctxt->child_index);
    if (ctxt->node->is<IR::AssignmentStatement>())
        return ctxt->child_index != 0;
    if (ctxt->node->is<IR::Argument>()) {
        // MethodCallExpression(Vector<Argument(Expression)>)
        if (!ctxt->parent || !ctxt->parent->parent || !ctxt->parent->parent->node)
            return false;
        if (auto *mc = ctxt->parent->parent->node->to<IR::MethodCallExpression>()) {
            auto type = mc->method->type->to<IR::Type_Method>();
            if (!type) {
                /* FIXME -- can't find the type of the method -- should be a BUG? */
                return true; }
            auto param = type->parameters->getParameter(ctxt->parent->child_index);
            return param->direction != IR::Direction::Out; } }
    if (ctxt->node->is<IR::IndexedVector<IR::StatOrDecl>>())
        return false;
    if (ctxt->node->is<IR::IfStatement>())
        return ctxt->child_index == 0;
    return true;
}

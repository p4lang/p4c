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

#ifndef MIDEND_EXPR_USES_H_
#define MIDEND_EXPR_USES_H_

#include "ir/ir.h"

/* Should this be a method on IR::Expression? */

/// Functor to check if an expression uses an lvalue.  The lvalue is specified as a
/// a cstring, which can be the name of a variable with optional field names and constant
/// array indexes for fields of headers or structs or unions or elements of stacks.
class exprUses : public Inspector {
    cstring look_for;
    const char *search_tail = nullptr;  // pointer into look_for for partial match
    bool result = false;
    bool preorder(const IR::Path *p) override {
        if (look_for.startsWith(p->name)) {
            search_tail = look_for.c_str() + p->name.name.size();
            if (*search_tail == 0 || *search_tail == '.' || *search_tail == '[')
                result = true; }
        return !result; }
    bool preorder(const IR::Primitive *p) override {
        if (p->name == look_for) result = true;
        return !result; }
    bool preorder(const IR::Expression *) override { return !result; }

    void postorder(const IR::Member *m) override {
        if (result && search_tail && *search_tail) {
            if (*search_tail == '.') search_tail++;
            if (cstring(search_tail).startsWith(m->member)) {
                search_tail += m->member.name.size();
                if (*search_tail == 0 || *search_tail == '.' || *search_tail == '[')
                    return; }
            search_tail = nullptr;
            if (!m->expr->type->is<IR::Type_HeaderUnion>()) {
                result = false; } } }
    void postorder(const IR::ArrayIndex *m) override {
        if (result && search_tail && *search_tail) {
            if (*search_tail == '.' || *search_tail == '[') search_tail++;
            if (isdigit(*search_tail)) {
                int idx = strtol(search_tail, const_cast<char **>(&search_tail), 10);
                if (*search_tail == ']') search_tail++;
                if (auto k = m->right->to<IR::Constant>()) {
                    if (k->asInt() == idx) return;
                } else {
                    return; } }
            result = false;
            search_tail = nullptr; } }
    void postorder(const IR::PathExpression *) override {}
    void postorder(const IR::Expression *) override { search_tail = nullptr; }

 public:
    exprUses(const IR::Expression *e, cstring n) : look_for(n) { e->apply(*this); }
    explicit operator bool () const { return result; }
};


#endif /* MIDEND_EXPR_USES_H_ */

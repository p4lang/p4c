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

class exprUses : public Inspector {
    cstring look_for;
    bool result = false;
    bool preorder(const IR::Path *p) override {
        if (p->name == look_for) result = true;
        return !result; }
    bool preorder(const IR::Member *m) override {
        if (m->toString() == look_for) result = true;
        return !result; }
    bool preorder(const IR::Primitive *p) override {
        if (p->name == look_for) result = true;
        return !result; }
    bool preorder(const IR::Expression *) override { return !result; }
 public:
    exprUses(const IR::Expression *e, cstring n) : look_for(n) { e->apply(*this); }
    explicit operator bool () { return result; }
};


#endif /* MIDEND_EXPR_USES_H_ */

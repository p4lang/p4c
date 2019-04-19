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

#ifndef MIDEND_HAS_SIDE_EFFECTS_H_
#define MIDEND_HAS_SIDE_EFFECTS_H_

#include "ir/ir.h"

/* Should this be a method on IR::Expression? */

class hasSideEffects : public Inspector {
    bool result = false;
    bool preorder(const IR::AssignmentStatement *) override { return !(result = true); }
    /* FIXME -- currently assuming all calls and primitves have side effects */
    bool preorder(const IR::MethodCallExpression *) override { return !(result = true); }
    bool preorder(const IR::Primitive *) override { return !(result = true); }
    bool preorder(const IR::Expression *) override { return !result; }
 public:
    explicit hasSideEffects(const IR::Expression *e) { e->apply(*this); }
    explicit operator bool () { return result; }
};


#endif /* MIDEND_HAS_SIDE_EFFECTS_H_ */

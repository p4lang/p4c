/*
Copyright 2025 Nvidia, Inc.

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

#ifndef FRONTENDS_P4_REMOVEOPASSIGN_H_
#define FRONTENDS_P4_REMOVEOPASSIGN_H_

#include "cloner.h"
#include "ir/ir.h"
#include "sideEffects.h"

namespace P4 {

class RemoveOpAssign : public Transform {
    template <class T>
    const IR::Node *doit(T *as) {
        prune();
        BUG_CHECK(!SideEffects::check(as->left, this), "side effects in LHS of %s", as);
        return new IR::AssignmentStatement(as->srcInfo, as->left->apply(CloneExpressions()),
                                           new typename T::BinOp(as->left, as->right));
    }

#define PREORDER(OP) \
    const IR::Node *preorder(IR::OP *as) override { return doit(as); }
    PREORDER(MulAssign)
    PREORDER(DivAssign)
    PREORDER(ModAssign)
    PREORDER(AddAssign)
    PREORDER(SubAssign)
    PREORDER(AddSatAssign)
    PREORDER(SubSatAssign)
    PREORDER(ShlAssign)
    PREORDER(ShrAssign)
    PREORDER(BAndAssign)
    PREORDER(BOrAssign)
    PREORDER(BXorAssign)
#undef PREORDER

    const IR::Node *preorder(IR::AssignmentStatement *s) override {
        prune();
        return s;
    }
    const IR::Node *preorder(IR::Expression *e) override {
        prune();
        return e;
    }
    const IR::Node *preorder(IR::Annotation *a) override {
        prune();
        return a;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_REMOVEOPASSIGN_H_ */

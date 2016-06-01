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

#ifndef _COMMON_CONSTANTFOLDING_H_
#define _COMMON_CONSTANTFOLDING_H_

#include <gmpxx.h>

#include "ir/ir.h"
#include "typeMap.h"
#include "resolveReferences/referenceMap.h"

namespace P4 {
// Can also be run before type checking, so it only uses types if
// they are available.
class ConstantFolding : public Transform {
 protected:
    const P4::ReferenceMap* refMap;  // if null no 'const' values can be resolved
    P4::TypeMap* typeMap;  // if null we have no types; updated for new constants
    bool typesKnown;
    // maps expressions and declarations to their constant values
    std::map<const IR::Node*, const IR::Expression*> constants;

 protected:
    // returns nullptr if the expression is not a constant
    const IR::Expression* getConstant(const IR::Expression* expr) const;
    void setConstant(const IR::Node* expr, const IR::Expression* result);

    const IR::Constant* cast(
        const IR::Constant* node, unsigned base, const IR::Type_Bits* type) const;
    const IR::Node* binary(const IR::Operation_Binary* op,
                           std::function<mpz_class(mpz_class, mpz_class)> func);
    // for == and != only
    const IR::Node* compare(const IR::Operation_Binary* op);
    const IR::Node* shift(const IR::Operation_Binary* op);

    enum class Result {
        Yes,
        No,
        DontKnow
    };

    Result setContains(const IR::Expression* keySet, const IR::Expression* constant) const;

 public:
    ConstantFolding(const ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), typesKnown(typeMap != nullptr) {
        visitDagOnce = true; setName("ConstantFolding");
    }

    using Transform::postorder;

    const IR::Node* postorder(IR::P4Program* program) override;
    const IR::Node* postorder(IR::Declaration_Constant* d) override;
    const IR::Node* postorder(IR::PathExpression* e) override;
    const IR::Node* postorder(IR::Cmpl* e) override;
    const IR::Node *postorder(IR::Neg *e) override;
    const IR::Node *postorder(IR::LNot *e) override;
    const IR::Node *postorder(IR::LAnd *e) override;
    const IR::Node *postorder(IR::LOr *e) override;
    const IR::Node *postorder(IR::Slice *e) override;
    const IR::Node *postorder(IR::Add *e) override;
    const IR::Node *postorder(IR::Sub *e) override;
    const IR::Node *postorder(IR::Mul *e) override;
    const IR::Node *postorder(IR::Div *e) override;
    const IR::Node *postorder(IR::Mod *e) override;
    const IR::Node *postorder(IR::BXor *e) override;
    const IR::Node *postorder(IR::BAnd *e) override;
    const IR::Node *postorder(IR::BOr *e) override;
    const IR::Node *postorder(IR::Equ *e) override;
    const IR::Node *postorder(IR::Neq *e) override;
    const IR::Node *postorder(IR::Lss *e) override;
    const IR::Node *postorder(IR::Grt *e) override;
    const IR::Node *postorder(IR::Leq *e) override;
    const IR::Node *postorder(IR::Geq *e) override;
    const IR::Node *postorder(IR::Shl *e) override;
    const IR::Node *postorder(IR::Shr *e) override;
    const IR::Node *postorder(IR::Concat *e) override;
    const IR::Node *postorder(IR::Member *e) override;
    const IR::Node *postorder(IR::Cast *e) override;
    const IR::Node* postorder(IR::SelectExpression* e) override;
};
}  // namespace P4

#endif /* _COMMON_CONSTANTFOLDING_H_ */

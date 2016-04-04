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
    P4::ReferenceMap* refMap;  // if null no 'const' values can be resolved
    P4::TypeMap* typeMap;  // if null we have no types
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

 public:
    ConstantFolding(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), typesKnown(typeMap != nullptr) {
        visitDagOnce = true;
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
};
}  // namespace P4

#endif /* _COMMON_CONSTANTFOLDING_H_ */

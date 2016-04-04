#ifndef _P4_STRENGTHREDUCTION_H_
#define _P4_STRENGTHREDUCTION_H_

#include "ir/ir.h"
#include "../common/resolveReferences/referenceMap.h"
#include "lib/exceptions.h"
#include "lib/cstring.h"
#include "ir/substitution.h"
#include "ir/substitutionVisitor.h"

namespace P4 {

class StrengthReduction final : public Transform {
    bool isOne(const IR::Expression* expr) const;
    bool isZero(const IR::Expression* expr) const;
    bool isFalse(const IR::Expression* expr) const;
    bool isTrue(const IR::Expression* expr) const;
    // if expr is a constant value that is a power of 2 return the log2 of it,
    // else return -1
    int isPowerOf2(const IR::Expression* expr) const;

 public:
    StrengthReduction() { visitDagOnce = true; }

    using Transform::postorder;

    const IR::Node* postorder(IR::BAnd* expr) override;
    const IR::Node* postorder(IR::BOr* expr) override;
    const IR::Node* postorder(IR::BXor* expr) override;
    const IR::Node* postorder(IR::LAnd* expr) override;
    const IR::Node* postorder(IR::LOr* expr) override;
    const IR::Node* postorder(IR::Sub* expr) override;
    const IR::Node* postorder(IR::Add* expr) override;
    const IR::Node* postorder(IR::Shl* expr) override;
    const IR::Node* postorder(IR::Shr* expr) override;
    const IR::Node* postorder(IR::Mul* expr) override;
    const IR::Node* postorder(IR::Div* expr) override;
    const IR::Node* postorder(IR::Mod* expr) override;
};

}  // namespace P4

#endif /* _P4_STRENGTHREDUCTION_H_ */

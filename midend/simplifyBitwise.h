#ifndef MIDEND_SIMPLIFYBITWISE_H_
#define MIDEND_SIMPLIFYBITWISE_H_

#include "ir/ir.h"
#include "lib/ordered_set.h"

namespace P4 {

/**
 * The purpose of this pass is to simplify the translation of the p4-16 translation of the
 * following p4_14 primitive:
 *     modify_field(hdr.field, parameter, mask);
 *
 * This gets translated to the following p4_16:
 *     hdr.field = hdr.field & ~mask | parameter & mask;
 *
 * which in term could be further simplified to a vector of simple assignments over slices.
 * This extensions could be folded to any combinations of Binary Ors and Binary Ands as long
 * as the masks never have any collisions.
 *
 * @pre none
 *
 * @todo: Extend the optimization to handle multiple combinations of masks
 */
class SimplifyBitwise : public PassManager {
    ordered_set<const IR::AssignmentStatement *> changeable;

    class Scan : public Inspector {
        SimplifyBitwise &self;
        mpz_class total_mask = 0;
        bool can_be_changed = false;

        using Inspector::preorder;
        using Inspector::postorder;

        bool preorder(const IR::Operation *op) override;
        bool preorder(const IR::Member *) override { return false; }
        bool preorder(const IR::Slice *) override { return false; }
        bool preorder(const IR::ArrayIndex *) override { return false; }
        bool preorder(const IR::Cast *) override { return false; }
        bool preorder(const IR::AssignmentStatement *as) override;
        bool preorder(const IR::BOr *bor) override;
        bool preorder(const IR::BAnd *band) override;
        bool preorder(const IR::Constant *cst) override;
        void postorder(const IR::AssignmentStatement *as) override;

     public:
        explicit Scan(SimplifyBitwise &s) : self(s) { setName("SimplifyBitwise::Scan"); }
    };

    class Update : public Transform {
        SimplifyBitwise &self;
        IR::Vector<IR::StatOrDecl> *slice_statements;
        const IR::AssignmentStatement *changing_as;
        mpz_class total_mask = 0;

     public:
        using Transform::preorder;
        using Transform::postorder;

        const IR::Node *preorder(IR::AssignmentStatement *as) override;
        const IR::Node *preorder(IR::BAnd *band) override;
        const IR::Node *postorder(IR::AssignmentStatement *as) override;

        explicit Update(SimplifyBitwise &s) : self(s) { setName("SimplifyBitwise::Update"); }
    };


 public:
    SimplifyBitwise() {
        setName("SimplifyBitwise");
        addPasses({ new Scan(*this), new Update(*this) });
    }
};

}  // namespace P4

#endif /* MIDEND_SIMPLIFYBITWISE_H_ */
